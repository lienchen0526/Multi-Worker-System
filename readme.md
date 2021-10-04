# Introduction
This is a network programming course project. In the project, we will design a multi-worker application that allows multiple users to login simotaneously. Each user that login to the app is allowed to:
1. Execute basic shell command
2. Communicate to each other via special command
    2.1 Direct Message
    2.2 Broadcast Message

In this design, there will encounter some challenge: 
1. Managing inter-process communication according to the command inputed from user.
2. Communicate among users by shared memory.

