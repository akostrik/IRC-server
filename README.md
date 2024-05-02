group: https://github.com/bakyt92/11_ft_irc/blob/master/docs/plan.md   
IRC = Internet Relay Chat  
* для группового общения, общаться через личные сообщения и обмениваться файлами
* на основе IRC разработано множество мессенджеров, такие как: ICQ, Skype, Discord, Telegram, Slack, etc...
* most public IRC servers don't usually set a connection password 
## How to run, an example:
In the first terminal:  
`make`  
`make run`
In the second terminal:   
`nc 0 6667`
`PASS 2`  
`NICK alice`  
`USER Alice 0 * Alice`  
`PRIVMSG alice Hello`  

## Сообщения (Internet Relay Chat Protocol)
* Серверы и клиенты создают сообщения на которые можно ответить, а можно и нет
* Если сообщение содержит правильные команды, то клиенту следует ответить, но это не означает, что всегда можно дождаться ответа
* `<reply> ::= <nick>['*'] '=' <hostname>` '*' обозначает, что клиент зарегистрирован как IRC-оператор
* Специальнае случаи:
  + `:Angel!localhost PRIVMSG Wiz`       a message from Angel to Wiz
  + `PRIVMSG jto@localhost :Hello`       a message to a user "jto" on server localhost
  + `PRIVMSG kalt%localhost`             a message to a user on the local server with username of "kalt", and connected from the localhost
  + `PRIVMSG Wiz!jto@localhost :Hello` a message to the user with nickname Wiz who is connected from the localhostand has the username "jto"

This project involves creating a simple IRC (Internet Relay Chat) server in C++. The goal is to develop a non-blocking server (no forking and using only one poll), that is fully compatible with an official IRC client, providing a platform for real-time text messaging in a network.
  
Examples of IRC technologies:  
* WeeChat
  
* POSIX specification for poll: If the value of fd is less than 0, events shall be ignored, and revents shall be set to 0 in that entry on return from poll()
* Wireshark utility
* `nc -v`

Info:  
* https://dev.twitch.tv/docs/irc
* https://www.quakenet.org/articles/102-press-release-irc-networks-under-systematic-attack-from-governments    
