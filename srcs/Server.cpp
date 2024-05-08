#include "Server.hpp"

bool sigReceived;

Server::Server(string port_, string pass_) : port(port_), pass(pass_), fdsToEraseNextIteration(set<int>()) {}

Server::~Server() {
  for(map<string, Ch*>::iterator it = chs.begin(); it != chs.end(); it++)
    delete it->second;
  for(map<int, Cli*> ::iterator it = clis.begin(); it != clis.end(); it++) {
    close(it->first);
    delete it->second;
  }
  clis.clear();
  polls.clear();
  close(fdForNewClis);
}

void Server::sigHandler(int sig) {
  cout << endl << "Signal Received\n";
  sigReceived = true;
  (void)sig;
}

// SOL_SOCKET = set parameters on the socket level
// 1 option = (optname, optval, optlen)
// a socket in non-blocking mode https://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking
void Server::init() {
  try {
    signal(SIGINT,  sigHandler); // catch ctrl + c
    signal(SIGQUIT, sigHandler); // catch ctrl + '\'
    signal(SIGTERM, sigHandler); // catch kill command
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  sigReceived = false;
  struct addrinfo hints, *listRes;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;      // the address family AF_INET =r IPv4
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;
  if(getaddrinfo(NULL, port.c_str(), &hints, &listRes))
    throw std::runtime_error("getaddrinfo error: [" + std::string(strerror(errno)) + "]");
  int optVal = 1;
  for(struct addrinfo* hint = listRes; hint != NULL; hint = hint->ai_next) {
    if((fdForNewClis = socket(hint->ai_family, hint->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC| SOCK_CLOEXEC, hint->ai_protocol)) < 0)
      throw std::runtime_error("function socket error: [" + std::string(strerror(errno)) + "]");
    if(setsockopt(fdForNewClis, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)))
      throw std::runtime_error("setsockopt error: [" + std::string(strerror(errno)) + "]");
    if(setsockopt(fdForNewClis, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal)))
      throw std::runtime_error("setsockopt error: [" + std::string(strerror(errno)) + "]");
    if(bind(fdForNewClis, hint->ai_addr, hint->ai_addrlen)) {
      close(fdForNewClis);
      hint->ai_next == NULL ? throw std::runtime_error("bind error: [" + std::string(strerror(errno)) + "]") : perror("bind error");
    }
    else
      break ;
  }
  freeaddrinfo(listRes);
  if(listen(fdForNewClis, SOMAXCONN))
    throw std::runtime_error("listen error: [" + std::string(strerror(errno)) + "]");
  struct pollfd pollForNewClis = {fdForNewClis, POLLIN, 0};
  polls.push_back(pollForNewClis);
}

// poll allows waiting for status updates on more than one socket in a single synchronous call
void Server::run() {
  std::cout << "Server is running. Waiting clients to connect >\n";
  while (sigReceived == false) {
    eraseUnusedClis();
    eraseUnusedChs();
    markPollsToSendMsgsTo();
    int countEvents = poll(polls.data(), polls.size(), 100);
    if (countEvents < 0)
      throw std::runtime_error("Poll error: [" + std::string(strerror(errno)) + "]");
    if(countEvents > 0)                                                           // there are some data in sockets
      for(std::vector<struct pollfd>::iterator poll = polls.begin(); poll != polls.end(); poll++) {
        if((poll->revents & POLLIN) && poll->fd == fdForNewClis) {                // there is a new client in fdServ
          addNewClient(*poll);
          break;
        }
        else if((poll->revents & POLLIN) && poll->fd != fdForNewClis)             // there is a msg to read in fdForMsgs
          receiveBufAndExecCmds(poll->fd);
        else if(poll->revents & POLLOUT)                                          // there is a msg to send
          sendPreparedResps(clis.at(poll->fd));
      }
  }
  std::cout << "Terminated\n";
}

// inet_ntoa() converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation
void Server::addNewClient(pollfd poll) {
  struct sockaddr sa;
  socklen_t       saLen = sizeof(sa);
  int fdForMsgs = accept(poll.fd, &sa, &saLen);                                    // every client as its own fd for msgs
  if(fdForMsgs == -1)
    return perror("accept");
  clis[fdForMsgs] = new Cli(fdForMsgs, inet_ntoa(((struct sockaddr_in*)&sa)->sin_addr));;
  struct pollfd pollForMsgs = {fdForMsgs, POLLIN, 0};
  polls.push_back(pollForMsgs);
  cout << "\n*** New cli (fd=" + static_cast< std::ostringstream &>((std::ostringstream() << std::dec << (fdForMsgs) )).str() + ")\n";
}

void Server::receiveBufAndExecCmds(int fd) {
  if(!(auth = clis.at(fd)))
    return ;
  vector<unsigned char> newBuf0(BUFSIZE); // std::vector is the recommended way of implementing a variable-length buffer in C++
  for(size_t i = 0; i < newBuf0.size(); i++)
    newBuf0[i] = '\0';
  int nbBytesReallyReceived = recv(auth->fd, newBuf0.data(), newBuf0.size() - 1, MSG_NOSIGNAL | MSG_DONTWAIT);
  if(nbBytesReallyReceived < 0)
    perror("recv");                                                           // the client may be here still
  else if(nbBytesReallyReceived == 0)                                         // the client has desappeared
    fdsToEraseNextIteration.insert(auth->fd);
  else {
    string newBufRecv = string(newBuf0.begin(), newBuf0.end());
    newBufRecv.resize(nbBytesReallyReceived);
    cout << infoBuf(newBufRecv);
    newBufRecv = auth->oldBufRecv + newBufRecv;
    std::vector<string> cmds = splitBufToCmds(newBufRecv);
    for(std::vector<string>::iterator cmd = cmds.begin(); cmd != cmds.end(); cmd++) {
      ar.clear();
      ar = splitCmdToArgs(*cmd);
      cout << infoCmd();
      execCmd();
    }
    cout << infoServ();
  }
}