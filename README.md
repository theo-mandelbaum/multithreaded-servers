This repository is a copy of my final project in my Computer Systems II course at James Madison University. The project specifications can be found [here](https://w3.cs.jmu.edu/kirkpams/361/projects/p3-dhcp.shtml)


# Background
As we began the second half of our semester in Computer Systems II, we shifted our focus to networked concurrency and connectivity. So, this included topics such as IP addressing, the internet model, transport-layer protocols, application layer broadcasting, and wireless connectivity. Accordingly, our second project in this course was to implement a simulation of the client side of a DHCP protocol, establishing network settings for a client. To accomplish this, we used BOOTP packet interpretation and UDP requests/responses. The project shown below build upon the client side DHCP implementation.

# Project Overview
For the third and final project in Computer Systems II, we were tasked with implementing the DHCP server, since we had already made the client. While this used many of the same topics and skills as the client (sending and receiving UDP messages, deciphering between DHCP signals), multihtreading was our last topic of the semester. Consequently, the DHCP server that we were implementing was a multithreaded server.
## Relevancy
The reason why this project is relevant to me now, seven months after completion, is because I have recently started exploring distributed systems. This project was my first real experience with concurrency in systems, although it was beginner multithreading.

# Technologies
## Language(s)
This project is entirely coded in C

# Features Implemented
- The first feature implemented in this project was server side response to DHCP Discover and DHCP Request. This involved creating a [server socket](https://github.com/theo-mandelbaum/multithreaded-servers/blob/d4a875da09ccdc204c7a21f5c4c7ccfdcde78358/src/server.c#L47), bind the socket to my client, [receive the message](https://github.com/theo-mandelbaum/multithreaded-servers/blob/d4a875da09ccdc204c7a21f5c4c7ccfdcde78358/src/server.c#L98) constructing a response to DHCP Discover and Request messages and sending said responses back to the client.

# How the project works


# What I Learned


# Testing and Evaluation


# What Would I do Differently?
