INFO-6016-01-19F
Network Programming

Authors:	Dylan Kirkby,
		Ivan Parkhomenko

GitHub:		https://github.com/KirkbyD/NP-Chat-Program
		Work done on branch 'working'

Project 1, Chat Program
A chat server and client that can handle multiple connections simultaneously. 
The server must be done in C++ and must use BSD sockets. The client can be a 
simple text based interface, or you can use a GUI. The client must also use 
BSD sockets. This assignment can be done in a group of up to 3 members.

Project created in visual studio 2019 community edition.
Runs in both debug & release, x64 & x86.

To run:
Use Visual Studio 2019 Community Edition.
Right click solution -> properties.
Set startup project to 'Multiple startup projects'
Move Server to top of the list, and set both Server and Client 'Action' to 'Start'

To open multiple instances of the client, while running the solution,
 right click client projects and select 'Debug' -> 'Start New Instance'


Buffer:		in dev/include
Protocol:	in dev/include
		Encodes integers by swapping endians via bitshifting.

Server:		Accepts 'Join', 'Leave', and 'Send' message types.
		Dispatches 'Recieve' type messages to clients.

Client:
	Commands
	Quotation marks in commands are unnecessary. 
	Room names will not handle spaces. Sending messages will.
		Join "Roomname"
		join "Roomname"
			Joins a Room on the server, creates it if not present.

		Leave "Roomname"
		leave "Roomname"
			Leaves room on the server, does not destroy empty rooms at this time.

		"Roomname" "Message message message."
			Broadcasts message to chosen room, if it exists.
