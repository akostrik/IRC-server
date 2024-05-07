/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akostrik <akostrik@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/03 21:38:52 by ufitzhug          #+#    #+#             */
/*   Updated: 2024/05/03 22:01:50 by akostrik         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
class Server;

int Server::execCmd() {
  if(ar.size() == 0)
    return 0;
  if(ar[0] == "PASS")
    return execPass();
  if(ar[0] == "NICK")
    return execNick();
  if(ar[0] == "USER")
    return execUser();
  if(ar[0] == "QUIT")
    return execQuit();
  if(ar[0] == "PING")
    return execPing();
  if(ar[0] == "CAP")
    return execCap();
  if(ar[0] == "WHOIS")
    return execWhois();
  if(ar[0] == "PRIVMSG")
    return execPrivmsg();
  if(ar[0] == "NOTICE")
    return execNotice();
  if(ar[0] == "JOIN")
    return execJoin();
  if(ar[0] == "PART")
    return execPart();
  if(ar[0] == "MODE" && ar.size() > 1 && ar[1][0] == '#')
    return execModeCh();
  if(ar[0] == "MODE" && ar.size() > 1 && ar[1][0] != '#')
    return execModeCli();
  if(ar[0] == "MODE" && ar.size() == 1)
    return prepareResp(auth, "461 MODE :Not enough parameters");                         // ERR_NEEDMOREPARAMS
  if(ar[0] == "TOPIC")
    return execTopic();
  if(ar[0] == "INVITE")
    return execInvite();
  if(ar[0] == "KICK")
    return execKick();
  return prepareResp(auth, "421 " + ar[0] + " " + " :is unknown mode char to me");      // ERR_UNKNOWNCOMMAND
}

// commands necessary for registration: PASS NICK USER CAP PING WHOIS
int Server::execPass() {
  if(auth->passOk)
    return prepareResp(auth, "462 :Unauthorized command (already registered)");          // ERR_ALREADYREGISTRED
  if(ar.size() < 2 || ar[1] == "")
    return prepareResp(auth, "461 PASS :Not enough parameters");                         // ERR_NEEDMOREPARAMS
  if(ar[1] != pass) {
    fdsToEraseNextIteration.insert(auth->fd);
    prepareResp(auth, "464 :" + auth->nick + " :Password incorrect");                     // ERR_PASSWDMISMATCH
  }
  auth->passOk = true;
  if(auth->passOk && auth->nick != "" && auth->uName != "" && !auth->capInProgress)
    prepareResp(auth, "001 :" + auth->nick + ": Welcome to the Internet Relay Network " + auth->nick + "!" + auth->uName + "@" + auth->host); // RPL_WELCOME
  return 0;
}

// not implemented here ERR_UNAVAILRESOURCE ERR_RESTRICTED ERR_NICKNAMEOLLISION
int Server::execNick() {
  if(ar.size() < 2 || ar[1].size() == 0)
    return prepareResp(auth, "431 :No nickname given");                                  // ERR_NONICKNAMEGIVEN
  if(ar[1].size() > 9 || ar[1].find_first_not_of("-[]^{}0123456789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM") != string::npos)
    return prepareResp(auth, "432 " + ar[1] + " :Erroneus nickname");                    // ERR_ERRONEUSNICKNAME
  for(std::map<int, Cli *>::iterator targ = clis.begin(); targ != clis.end(); targ++)
    if(toLower(ar[1]) == toLower(targ->second->nick))
      return prepareResp(auth, "433 " + auth->nick + " " + ar[1] + " :Nickname is already in use"); // ERR_NICKNAMEINUSE
  auth->nick = ar[1];
  if (auth->nick != "" && auth->uName != "" && auth->passOk && !auth->capInProgress) 
    prepareResp(auth, "001 :" + auth->nick + ": Welcome to the Internet Relay Network " + auth->nick + "!" + auth->uName + "@" + auth->host); // RPL_WELCOME
  return 0;
}

int Server::execUser() {
  if(ar.size() < 5)
    return prepareResp(auth, "461 USER :Not enough parameters");                         // ERR_NEEDMOREPARAMS 
  if(auth->uName != "")
    return prepareResp(auth, ":username can not be empty");                              
  auth->uName = ar[1];
  auth->rName = ar[4];
  if(auth->nick != "" && auth->passOk && !auth->capInProgress)
    prepareResp(auth, "001 :" + auth->nick + ": Welcome to the Internet Relay Network " + auth->nick + "!" + auth->uName + "@" + auth->host); // RPL_WELCOME
  return 0;
}

int Server::execCap() {
  if(ar.size() < 2)
    return 0;
  if(ar[1] == "LS") {
    auth->capInProgress = true;
    return prepareResp(auth, "CAP * LS :");
  }
  if(ar[1] == "END" && auth->passOk && auth->nick != "" && auth->uName != "") {
    auth->capInProgress = false;
    return prepareResp(auth, "001 :" + auth->nick); // :Welcome to the Internet Relay Network " + auth->nick + "!" + auth->uName + "@" + auth->host); // RPL_WELCOME целиком не отпраляется, но для irssi это кажется не проблема
  }
  return 0;
}

int Server::execPing() {
  return prepareResp(auth, "PONG");
}

// not implemented here: RPL_WHOISCHANNELS RPL_WHOISOPERATOR RPL_AWAY RPL_WHOISIDLE
int Server::execWhois() {
  if(ar.size() < 2)
    return prepareResp(auth, "431 :No nickname given");                                  // ERR_NONICKNAMEGIVEN
  std::vector<string> nicks = splitArgToSubargs(ar[1]);
  for(vector<string>::iterator nick = nicks.begin(); nick != nicks.end(); nick++)
    if(getCli(*nick) == NULL)
      prepareResp(auth, "401 :" + *nick + " No such nick");                              // ERR_NOSUCHNICK
    else {
      prepareResp(auth, "311 " + *nick + " " + getCli(*nick)->uName + " " + getCli(*nick)->host + " * :" + getCli(*nick)->rName); // RPL_WHOISUSER
      prepareResp(auth, "318 " + *nick + " :End of WHOIS list");                         // RPL_ENDOFWHOIS
    }
  return 0;
}

// not implemented here: ERR_CANNOTSENDTOCHAN ERR_NOTOPLEVEL ERR_WILDTOPLEVEL RPL_AWAY ERR_TOOMANYTARGETS
int Server::execPrivmsg() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(ar.size() == 1) 
    return prepareResp(auth, "411 :No recipient given (" + ar[0] + ")");                 // ERR_NORECIPIENT 
  if(ar.size() == 2)
    return prepareResp(auth, "412 :No text to send");                                    // ERR_NOTEXTTOSEND 
  vector<string> targs = splitArgToSubargs(ar[1]);
  if((set<std::string>(targs.begin(), targs.end())).size() < targs.size() || targs.size() > MAX_NB_TARGETS)
    return prepareResp(auth, "407 " + ar[1] + " not valid recipients");                  // ERR_TOOMANYTARGETS -> how many?
  for(vector<string>::iterator targ = targs.begin(); targ != targs.end(); targ++)
    if((*targ)[0] == '#' && chs.find(toLower(*targ)) == chs.end())
      prepareResp(auth, "401 " + *targ + " :No such nick/channel " + (*targ));                        // ERR_NOSUCHNICK
    else if((*targ)[0] == '#' && getCliOnCh(auth->nick, *targ) == NULL)
      prepareResp(auth, "404 :" + auth->nick + "!" + auth->uName + "@127.0.0.1 " + *targ + " :Cannot send to channel"); 
    else if((*targ)[0] == '#' && getCliOnCh(auth->nick, *targ) != NULL)
      prepareRespExceptAuthor(getCh(*targ), ":" + auth->nick + "!" + auth->uName + "@127.0.0.1 PRIVMSG " + ar[1] + " :" + ar[2]);
    else if((*targ)[0] != '#' && !getCli(*targ))
      prepareResp(auth, "401 " + *targ + " :No such nick/channel " + (*targ));                      // ERR_NOSUCHNICK
    else if((*targ)[0] != '#')
      prepareResp(getCli(*targ), ":" + auth->nick + "!" + auth->uName + "@127.0.0.1 PRIVMSG " + ar[1] + " :" + ar[2]);
  return 0;
}

int Server::execNotice() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "" || ar.size() < 3)
    return 0;
  vector<string> targs = splitArgToSubargs(ar[1]);
  for(vector<string>::iterator targ = targs.begin(); targ != targs.end(); targ++)
    if((*targ)[0] == '#' && getCh(*targ) != NULL)
      prepareRespExceptAuthor(getCh(*targ), ":" + auth->nick + "!" + auth->uName + "@127.0.0.1 NOTICE " + ar[1] + " :\033[1;31m" + ar[2] + "\033[0m");
    else if((*targ)[0] != '#' && getCli(*targ) != NULL)
      prepareResp(getCli(*targ), ":" + auth->nick + "!" + auth->uName + "@127.0.0.1 NOTICE " + ar[1] + " :\033[1;31m" + ar[2] + "\033[0m");
  return 0;
}

// not implemented here: ERR_BANNEDFROMCHAN ERR_BADCHANMASK ERR_NOSUCHCHANNEL ERR_UNAVAILRESOURCE
int Server::execJoin() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(ar.size() < 2)
    return prepareResp(auth, "461 JOIN :Not enough parameters");                         // ERR_NEEDMOREPARAMS 
  if(ar.size() == 2 && ar[1] == "0") {
    ar[1] = "";
    for(map<string, Ch*>::iterator ch = chs.begin(); ch != chs.end(); ch++)
      if(getCliOnCh(auth, ch->first) != NULL)
        ar[1] += ch->first + ",";
    if(ar[1].size() > 0)
      ar[1].resize(ar[1].size() - 1);
    return execPart();
  }
  vector<string> chNames = splitArgToSubargs(ar[1]);
  if ((set<std::string>(chNames.begin(), chNames.end())).size() < chNames.size() || chNames.size() > MAX_NB_TARGETS)
    return prepareResp(auth, ar[1] + " :407 recipients. Too many targets.");             // ERR_TOOMANYTARGETS
  vector<string> passes = ar.size() >= 3 ? splitArgToSubargs(ar[2]) : vector<string>();
  for(vector<string>::iterator chName = chNames.begin(); chName != chNames.end(); chName++) {
    if (nbChannels(auth) > MAX_CHS_PER_USER - 1)
      return prepareResp(auth, "405 " + ar[1] + " :You have joined too many channels");  // ERR_TOOMANYCHANNELS
    else if(chName->size() <= 1 || chName->size() > 200 || (*chName)[0] != '#' || chName->find_first_of("0\\") != string::npos)
      prepareResp(auth, "403 " + *chName + " :No such channel");                         // ERR_NOSUCHCHANNEL ?
    else if(getCliOnCh(auth->nick, *chName) != NULL) 
      prepareResp(auth, ":you are already on channel " + *chName);                       // already on channel
    else {
      if(getCh(*chName) == NULL)
        chs[*chName] = new Ch(auth);
      string pass = "";
      if (passes.size() > 0) {
        pass = *(passes.begin());
        passes.erase(passes.begin());
      }
      if(getCh(*chName)->pass != "" && pass != getCh(*chName)->pass)
        prepareResp(auth, "475 " + *chName + " :Cannot join channel (+k)");              // ERR_BADCHANNELKEY
      else if(getCh(*chName)->size() >= getCh(*chName)->limit)
        prepareResp(auth, "471 " + *chName + " :Cannot join channel (+l)");              // ERR_CHANNELISFULL
      else if(getCh(*chName)->optI && auth->invits.find(*chName) == auth->invits.end())
        prepareResp(auth, "473 " + *chName + " :Cannot join channel (+i)");              // ERR_INVITEONLYCHAN
      else {
        getCh(*chName)->clis.insert(auth);
        if(auth->invits.find(*chName) != auth->invits.end())
          auth->invits.erase(*chName);
        prepareRespAuthorIncluding(getCh(*chName), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " JOIN " + *chName); // ok channel
        if(getCh(*chName)->topic != "")
          prepareResp(auth, "332 " + auth->nick + " " + *chName + " :" + getCh(*chName)->topic); // RPL_TOPIC                      // ok channel
        prepareRespAuthorIncluding(getCh(*chName), ":localhost 353 " + auth->nick + "!" + auth->uName + "@127.0.0.1" + " = " + *chName + " :" + users(getCh(*chName)));
        // prepareResp(auth, ":localhost 353 " + auth->nick + "!" + auth->uName + "@127.0.0.1" + " = " + *chName + " :" + users(getCh(*chName)));               // RPL_NAMREPLY
      } 
    }
  }
  return 0;
}

int Server::execPart() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(ar.size() < 2)
    return prepareResp(auth, "461 PART :Not enough parameters");                         // ERR_NEEDMOREPARAMS
  vector<string> chNames = splitArgToSubargs(ar[1]);
  for(vector<string>::iterator chName = chNames.begin(); chName != chNames.end(); chName++)
    if(getCh(*chName) == NULL)
      prepareResp(auth, "403 " + *chName + " :No such channel");                         // ERR_NOSUCHCHANNEL
    else if(getCliOnCh(auth, *chName) == NULL)
      prepareResp(auth, "442 " + *chName + " :You're not on that channel");              // ERR_NOTONCHANNEL
    else {
      if (ar.size() > 2) {
        prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " PART " + ar[1] + " :" + ar[2]);
      }
      else {
        prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " PART " + ar[1]);
      }
      eraseCliFromCh(auth->nick, *chName);
    }
  return 0;
}

// not implemented here ERR_BADCHANMASK
int Server::execKick() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(ar.size() < 3)
    return prepareResp(auth, "461 KICK :Not enough parameters");                         // ERR_NEEDMOREPARAMS
  std::vector<string> chNames = splitArgToSubargs(ar[1]);
  std::vector<string> targs = splitArgToSubargs(ar[2]);
  for(vector<string>::iterator chName = chNames.begin(); chName != chNames.end(); chName++) {
    if(getCh(*chName) == NULL)
      prepareResp(auth, "403 " + *chName + " :No such channel");                         // ERR_NOSUCHCHANNEL
    else if(getCliOnCh(auth, *chName) == NULL)
      prepareResp(auth, "442 " + *chName + " :You're not on that channel");              // ERR_NOTONCHANNEL
    else if(getAdmOnCh(auth, *chName) == NULL)
      prepareResp(auth, "482 " + *chName + " :You're not channel operator");             // ERR_CHANOPRIVSNEEDED
    else {
      for(vector<string>::iterator targ = targs.begin(); targ != targs.end(); targ++) {
        if(getCliOnCh(*targ, *chName) == NULL)
          prepareResp(auth, "441 " + *targ + " " + *chName + " :They aren't on that channel"); // ERR_USERNOTINCHANNEL 
        else {
          if (ar.size() > 3) {
            prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " KICK " + ar[1] + " " + ar[2] + " :" + ar[3]);
          }
          else {
            prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " KICK " + ar[1] + " " + ar[2]);
          }
          eraseCliFromCh(*targ, *chName);
        }
      }
    }
  }
  return 0;
}

// not implemented here RPL_AWAY
int Server::execInvite() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(ar.size() < 3)
    return prepareResp(auth, "461 INVITE :Not enough parameters");                       // ERR_NEEDMOREPARAMS
  if(getCli(ar[1]) == NULL)
    return prepareResp(auth, "401 " + ar[1] + " :No such nick");                         // ERR_NOSUCHNICK
  if(getCh(ar[2]) == NULL)
    return prepareResp(auth, "403 " + ar[1] + " :No such channel");                      // ERR_NOSUCHCHANNEL
  if(getCh(ar[2]) != NULL && getCliOnCh(auth, ar[2]) == NULL)
    return prepareResp(auth, "442 " + ar[2] + " :You're not on that channel");           // ERR_NOTONCHANNEL
  if(getCh(ar[2]) != NULL && getAdmOnCh(auth, ar[2]) == NULL && getCh(ar[2])->optI == true)
    return prepareResp(auth, "482 " + ar[2] + " :You're not channel operator");          // ERR_CHANOPRIVSNEEDED
  if(getCh(ar[2]) != NULL && getCliOnCh(ar[1], ar[2]) != NULL)
    return prepareResp(auth, "443 " + ar[1] + " " + ar[2] + " :is already on channel");  // ERR_USERONCHANNEL
  if(getCh(ar[2]) == NULL)
    chs[ar[2]] = new Ch(auth);
  getCli(ar[1])->invits.insert(ar[2]);
  prepareResp(auth, ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " 341 " + ar[1] + " " + ar[2]);                                       // RPL_INVITING
  prepareResp(getCli(ar[1]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " INVITE " + ar[1] + " " + ar[2]);  
  return prepareRespAuthorIncluding(getCh(ar[2]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " INVITE " + ar[1] + " " + ar[2]);
}

// not implemented here ERR_NOCHANMODES
int Server::execTopic() {
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(ar.size() < 2)
    return prepareResp(auth, "461 TOPIC :Not enough parameters");                        // ERR_NEEDMOREPARAMS
  if(getCh(ar[1]) == NULL)
    return prepareResp(auth, "403 " + ar[1] + " :No such channel");                      // ERR_NOSUCHCHANNEL
  if(getCliOnCh(auth, ar[1]) == NULL)
    return prepareResp(auth, "442 " + ar[1] + " :You're not on that channel");           // ERR_NOTONCHANNEL
  if(getAdmOnCh(auth, ar[1]) == NULL && getCh(ar[1])->optT == true)
    return prepareResp(auth, "482 " + ar[1] + " :You're not channel operator");          // ERR_CHANOPRIVSNEEDED
  if(ar.size() == 2 && getCh(ar[1])->topic == "")  
    return prepareResp(auth, "331 :" + auth->nick + " " + ar[1] + " :No topic is set"); // RPL_NOTOPIC
  if(ar.size() == 2 && getCh(ar[1])->topic != "")  
    return prepareResp(auth, "332 :" + auth->nick + " " + ar[1] + " :" + getCh(ar[1])->topic);              // RPL_TOPIC
  if(ar.size() >= 3 && (ar[2] == ":" || ar[2] == "")) { 
    getCh(ar[1])->topic = "";
    prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "@localhost" + " TOPIC " + ar[1] + " :No topic is set");
    return prepareResp(auth, "331 :" + auth->nick + " " + ar[1] + " :No topic is set"); // RPL_NOTOPIC
  }
  getCh(ar[1])->topic = ar[2];
  prepareResp(auth, "332 :" + auth->nick + " " + ar[1] + " :1" + getCh(ar[1])->topic);                        // RPL_TOPIC
  return prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "@localhost" + " TOPIC " + ar[1] + " :2" + getCh(ar[1])->topic);
}

int Server::execQuit() {
  for(map<int, Cli*>::iterator cli = clis.begin(); cli != clis.end(); cli++) {
    if (ar.size() == 2)
      prepareResp(cli->second, cli->second->nick + "!" + cli->second->uName + "@" + cli->second->host + " QUIT :Quit:" + ar[1]);
    else 
      prepareResp(cli->second, cli->second->nick + "!" + cli->second->uName + "@" + cli->second->host + " QUIT :Quit:");
  }
  fdsToEraseNextIteration.insert(auth->fd);
  return 0;
}

// not implemented here: ERR_NOCHANMODES RPL_BANLIST RPL_ENDOFBANLIST RPL_EXCEPTLIST RPL_ENDOFEXCEPTLIST RPL_INVITELIST RPL_ENDOFINVITELIST RPL_UNIQOPIS (creator of the channel)
int Server::execModeCh() {  //  +i   -i   +t   -t   -k   -l   +k mdp   +l 5   +o alice,bob   -o alice,bob
  if(!auth->passOk || auth->nick== "" || auth->uName == "")
    return prepareResp(auth, "451 " + auth->nick + " :User not logged in" );              // ERR_NOTREGISTERED
  if(getCh(ar[1]) == NULL)
    return prepareResp(auth, "403 " + ar[1] + " :No such channel");                      // ERR_NOSUCHCHANNEL
  if(getCliOnCh(auth, ar[1]) == NULL)
    return prepareResp(auth, "442 " + ar[1] + " :You're not on that channel");           // ERR_NOTONCHANNEL
  if(getAdmOnCh(auth, ar[1]) == NULL)
    return prepareResp(auth, "482 " + ar[1] + " :You're not channel operator");          // ERR_CHANOPRIVSNEEDED
  if(ar.size() == 2)
    return prepareResp(auth, "MODE " + ar[1] + " " + mode(getCh(ar[1])));                // RPL_CHANNELMODEIS
  for(size_t i = 2; i < ar.size(); ) {
    string opt = ar[i];
    string val = (ar.size() >= i + 2 && (opt == "+k" || opt == "+l" || opt == "+o" || opt == "-o")) ? ar[3] : "";
    if((opt == "+o" || opt == "-o") && val != "") {
      vector<string> vals = splitArgToSubargs(val);
      for(vector<string>::iterator it = vals.begin(); it != vals.end(); it++)
        execModeOneOoption(opt, *it);
    }
    else 
      execModeOneOoption(opt, val);
    i++;
    if (opt == "+k" || opt == "+l" || opt == "+o" || opt == "-o")
      i++;
  }
  return 0;
}

int Server::execModeOneOoption(string opt, string val) {
  char *notUsed;
  if(opt != "+i" && opt != "-i" && opt != "+t" && opt != "-t" && opt != "+l" && opt != "-l" && opt != "+k" && opt != "-k" && opt != "+o" && opt != "-o")
    return prepareResp(auth, "472 " + opt + " :is unknown mode char to me for " + val); // ERR_UNKNOWNMODE
  if(val == "" && opt != "+i" && opt != "-i" && opt != "+t" && opt != "-t" && opt != "-l" && opt != "-k" && opt != "+o" && opt != "-o")
    return prepareResp(auth, "461 MODE :Not enough parameters");                          // ERR_NEEDMOREPARAMS
  if(opt == "+k" && getCh(ar[1])->pass != "")
    return prepareResp(auth, "467 " + ar[1] + " :Channel key already set");               // ERR_KEYSET
  if(opt == "+l" && (atoi(ar[3].c_str()) < static_cast<int>(0) || static_cast<unsigned int>(atoi(ar[3].c_str())) > std::numeric_limits<unsigned int>::max()))
    return prepareResp(auth, "472 " + auth->nick + " " + opt + " :is unknown mode char to me"); // ERR_UNKNOWNMODE
  if((opt == "+o" || opt == "-o") && getCliOnCh(val, ar[1]) == NULL)
    return prepareResp(auth, "441 " + val + " " + ar[1] + " :They aren't on that channel!!!"); // ERR_USERNOTINCHANNEL
  if(opt == "+l" && (atoi(val.c_str()) < static_cast<int>(0) || static_cast<unsigned int>(atoi(ar[3].c_str())) > std::numeric_limits<unsigned int>::max()))
    return prepareResp(auth, "? " + opt + " " + val + " MODE :bad option value");          // ?
  if(opt == "+i")
    getCh(ar[1])->optI = true;
  else if(opt == "-i")
    getCh(ar[1])->optI = false;
  else if(opt == "+t")
    getCh(ar[1])->optT = true;
  else if(opt == "-t")
    getCh(ar[1])->optT = false;
  else if(opt == "-l")
    getCh(ar[1])->limit = std::numeric_limits<unsigned int>::max();
  else if(opt == "+k")
    getCh(ar[1])->pass = val; // no limitations for a pass ?
  else if(opt == "-k")
    getCh(ar[1])->pass = "";
  else if(opt == "+l" && atoi(val.c_str()) >= static_cast<int>(0) && static_cast<unsigned int>(atoi(val.c_str())) <= std::numeric_limits<unsigned int>::max())
    getCh(ar[1])->limit = static_cast<int>(strtol(val.c_str(), &notUsed, 10));
  else if(opt == "+o")
    getCh(ar[1])->adms.insert(getCli(val));
  else if(opt == "-o" && getAdmOnCh(val, ar[1]) != NULL)
    getCh(ar[1])->adms.erase(getCli(val));
  return prepareRespAuthorIncluding(getCh(ar[1]), ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " MODE " + ar[1] + " " + opt);
}

// not implemented ERR_UMODEUNKNOWNFLAG
// partially implemented RPL_UMODEIS
int Server::execModeCli() {
  if(getCli(ar[1]) == NULL)
    return prepareResp(auth, "401 :" + ar[1] + " No such nick");                           // ERR_NOSUCHNICK
  if(getCli(ar[1])->nick != auth->nick)
    return prepareResp(auth, "502 :" + ar[1] + " :Can't change mode for other users");     // ERR_USERSDONTMATCH
  if(ar.size() == 2)
    return prepareResp(auth, "221 :" + auth->nick + " ");                                   // RPL_UMODEIS
  if(ar.size() > 3 && ar[2] == "+i")
    return prepareResp(auth, ":" + auth->nick + "!" + auth->uName + "@" + auth->host + " MODE " + ar[1] + " +i ");
  return 0;
}
