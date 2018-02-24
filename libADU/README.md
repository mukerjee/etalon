# libADU

libADU is a interposition library that wraps ```write```, ```send```,
```shutdown```, and ```close``` to inform our reconfigurable switch emulator of
demand waiting on an endhost. ```write``` and ```send``` send a message to the
reconfigurable switch indicating how much data was just passed into the socket
buffer, and ```shutdown``` and ```close``` send a message to the reconfigurable
switch indicating remaining data for this flow has been expunged.
