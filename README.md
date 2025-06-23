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
- The first feature implemented in this project was server side echo response to DHCP Discover and DHCP Request. This involved creating a [server socket](https://github.com/theo-mandelbaum/multithreaded-servers/blob/d4a875da09ccdc204c7a21f5c4c7ccfdcde78358/src/server.c#L47), bind the socket to my client, [receive the message](https://github.com/theo-mandelbaum/multithreaded-servers/blob/d4a875da09ccdc204c7a21f5c4c7ccfdcde78358/src/server.c#L98) constructing a response to DHCP Discover and Request messages and sending said responses back to the client.
- The next step is to extend the DHCO echo server to handle DHCP Requests that warrant a DHCP Acknowledge response. This takes place when the IP address and the server ID match.
- The next big feature implemented was the tracking and releasing of assignments. For each DHCP message that is sent, the [IP addresses are tracked using an ip_records array](https://github.com/theo-mandelbaum/multithreaded-servers/blob/d4a875da09ccdc204c7a21f5c4c7ccfdcde78358/src/server.c#L533). The IP addresses are tracked in order to support interleaved  messages and re-assigning addresses.
  - When working with multiple clients, one client might send their original DHCP Discover followed by another client sending their DHCP Dicscover. After the second client has sent a message, the first client might want to send it's DHCP Request. To do this, the IP address of the first client must be tracked.
  - For client re-assignment, a client will send a DHCP Release message to allow that IP address to be re-assigned. By keeping an array of records, it is possible for the IP address to be removed and re-assigned to a different client.
- The final feature is the most relevant piece of this project—multithreading. Instead of processing a DHCP Discover message right away, my main thread [launches a helper thread](https://github.com/theo-mandelbaum/multithreaded-servers/blob/3484024d0cd59e5161cf22d5a9144f5f7233384f/src/server.c#L627), then goes back to waiting on another message to be sent. This way, requests are all handled in separate threads, then joined at the end, allowing multiple requests to be handled at the same time with proper distributiong among threads.

# How the project works


# What I Learned


# Testing and Evaluation


# What Would I do Differently?
