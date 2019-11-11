INFO-6016-01-19F
Network Programming

Authors:	Dylan Kirkby,
		Ivan Parkhomenko,
		Brian Cowan

GitHub:		https://github.com/KirkbyD/NP-Chat-Program
		Work done on branch 'working'

Project 2, Chat Program
A chat server and client that can handle multiple connections simultaneously. 
The server must be done in C++ and must use BSD sockets. The client can be a 
simple text based interface, or you can use a GUI. The client must also use 
BSD sockets. This assignment can be done in a group of up to 3 members.

Project created in visual studio 2019 community edition.
Runs ONLY IN release x64!!!!!

To run:
Use Visual Studio 2019 Community Edition.
Right click solution -> properties.
Set startup project to 'Multiple startup projects'
Arrange them in the next order:
1) Authentication Server
2) Server
3) Client

To open multiple instances of the client, while running the solution, 
right click client projects and select 'Debug' -> 'Start New Instance'

To prepare your database for chat application run commands from .sql files in folder "MySQL DB" in the next order:
1) "authentication server database drop & clear.sql"
2) "authentication server database setup.sql"
3) "authentication user creation and privleges script.sql"

Buffer:		in dev/include
Protocol:	in dev/include
		Encodes integers by swapping endians via bitshifting.

Auth Server:	Accepts command types 'Authenticate' with username or email, and 'Register'

Chat Server:	Accepts 'Join', 'Leave', and 'Send' message types.
		Dispatches 'Recieve' type messages to clients.

		Now also recieves message types like 'Authenticate', 'Register', and 'Logout' from the client.
		These are parsed then sent to the Auth server via google protobuff inside our custom buffer for the length prefix header.

		Now also recieves from the Auth server
		Messages type: 
			'RegistrationSuccess', 'RegistrationFailure', 
			'AuthenticationSuccess', 'AuthenticationFailure',
			'LogoutSuccess', and 'DisconnectFailure'

Client:
	Commands:
	Register username emil password
		Attempts to register account to database
	
	Authenticate usernanme/email password
		Attempts to log in to with an account.
		The presence of an '@' character denotes that an email is being used.

	Require Authentication:
	Join [Roomname]
		Joins a Room on the server, creates it if not present.
		Will not handle spaces in room name.

	Leave [Roomname]
		Leaves room on the server, does not destroy empty rooms at this time.

	[Roomname] [Message]
		Broadcasts message to chosen room, if it exists and the user is authenticated.
		
	Logout
		Attempts to log the client out if they are authenticated.
