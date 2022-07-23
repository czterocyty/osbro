# Brother DCP-7020L

SANE drivers stopped working after upgrade to Ubuntu 20.04 on amd64 architecture.

I received some USB bus errors, as it seems that original, official Brother driver was 
written against libusb-0.1 and magically stopped working.

Secondly, I could not compile original sane/scanner drivers against Raspberry Pi (arm architecture),
as there are too many errors, and my idea of providing network backend of SANE at Pi fell through.

Till some time, I had the following setup at home:

Brother DCP was connected via USB to Raspberry Pi, as served as network driver natively by CUPS.
Due to missing sane driver on Raspberry Pi, I used Linux kernel feature _USB over IP_ to
bypass scanner to Ubuntu (amd64) workstation and was able to scan from there using SANE.

I tried to write my own SANE driver to provide scanning at my house using the scanner attached to Raspberry Pi.

This task seems to be hard. So I just followed the recommendation to start from some command line 
program.

I have no commercial experience of C programming, only tried it at university, years ago.

Based on official Brother scanner driver and some others work I wrote some simple C program
that used newer libusb-1.0 library and writes a raw file. 

## Credits

* official Brother drivers, brscan2
* [tm4n](https://github.com/tm4n) and its repository: https://github.com/tm4n/dcp7030
