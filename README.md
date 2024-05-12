This project involves creating a simple IRC (Internet Relay Chat) server in C++. The goal is to develop a non-blocking server (no forking and using only one poll), that is fully compatible with an official IRC client, providing a platform for real-time text messaging in a network.
* Examples of IRC technologies: ICQ, Skype, Discord, Telegram, Slack, etc...

## Run using the terminal:
In the first terminal:  
`make`  
`make run`  
In the second terminal:   
`nc localhost 6667`  
`PASS myPass`  
`NICK alice`  
`USER Alice localhost * Alice`  
`PRIVMSG alice Hello`  

## Run using irssi client:
In the terminal:  
`make`  
`make run`  
Install and run irssi :
`/connect localhost 6667 myPass alice`  
`/msg alice Hello`  

## Инфо
* there is a problem in this project : the chain operator does MODE #ch +i, MODE #ch -i, then another client JOIN #ch
* https://modern.ircdocs.horse/ (!)
* https://ircgod.com/ (!)
* https://github.com/levensta/IRC-Server
* https://github.com/marineks/Ft_irc
* https://github.com/miravassor/irc
* https://masandilov.ru/network/guide_to_network_programming – гайд по сетевому программированию
* https://ncona.com/2019/04/building-a-simple-server-with-cpp/ – про сокеты и TCP-сервер на C++
* https://www.ibm.com/docs/en/i/7.3?topic=designs-example-nonblocking-io-select non-blocking I/O
* http://www.kegel.com/dkftpbench/nonblocking.html non-blocking I/O
* https://medium.com/@afatir.ahmedfatir/small-irc-server-ft-irc-42-network-7cee848de6f9  
* https://irc.dalexhd.dev/index.html  
* https://beej.us/guide/bgnet/html/  
* https://rbellero.notion.site/School-21-b955f2ea0c1c45b981258e1c41189ca4   
* https://www.notion.so/FT_IRC-c71ec5dfc6ae46509fb281acfb0e9e29?pvs=4  
* https://stackoverflow.com/questions/27705753/should-i-use-pass-before-nick-user/27708209#27708209
* https://dev.twitch.tv/docs/irc
* https://www.quakenet.org/articles/102-press-release-irc-networks-under-systematic-attack-from-governments
