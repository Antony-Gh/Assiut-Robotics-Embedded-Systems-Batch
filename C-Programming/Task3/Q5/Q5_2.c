/*

A union is useful when a variable needs to represent different kinds of data at different times, but only one kind is active at once.

Examples include:

A sensor reading that may be an integer, floating-point value, or status code.

A communication packet with different message payload formats.

A state-dependent data record in a memory-constrained device.

A union’s size is at least large enough for its largest member, subject to alignment.
To use it safely, keep a separate tag or enum indicating which member is currently valid.
Avoid assuming that writing one member and reading another performs a portable conversion.

*/