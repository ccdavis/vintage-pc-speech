This zip file contains a snapshot of the archived source on my PC. I haven't verified the contents of the 
zip file nor it's consistency. 

This is a potpourri of files that may or may not be useful, hopefully if you can reproduce my development 
environment you can try compiling it again.

My development environment included a public domain macro assembler and Borland's TC V3.0 compiler, 
which ran under a DOS session on IBM's OS/2 operating system.

OS/2 was a boon for driver development as one could "crash" the system and continue development by 
closing down the DOS session and kicking off another DOS session.

I can't recall all the details of where and when I stopped development but it was many years ago. 
The last thing I remember was wanting to distribute the source so I was stuffing around with file 
talk.asm renaming it to c0.asm but  trying to make the driver development more like writing a 
normal C program.

A confession that I want to make was that I wanted to keep the assembler interface to a minimum 
because I find it harder to deal with than C or C++. So I set up an enormous stack and effectively 
wrote the whole driver including the hardware interface in C.

The only assembly in the C components is in PlayPhoneme function in module phoneme.c, where the software 
interfaces the hardware. 

This driver allowed me to experiment with the timings and see how they effected the quality of speech.

I'd better get back to my real work.

This program comes under a licence based upon the GNU concept have a read of the GPL.txt file before 
accepting this package.

Jon Hornstein (24 February 1999)
sultan@connexus.net.au
jon.hornstein@eudoramail.com
http:/home.connexus.net.au/~sultan


