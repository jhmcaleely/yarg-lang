# XIP Library Format

cyarg targets microcontrollers which have read only firmware storage that is memory mapped, and design for execute-in-place usage.

A 'file format' is needed to describe the location of yarg code and resources in this XIP memory, and also to be available as a file when cyarg runs on a host OS.

The file format consists of a set of concatenated nodes, with a header and index to each node at the start.

Within this format, the first node is guaranteed to be the index node.

Future: Add fixed nodes for bootstrap yarg files, and a directory of names for the nodes.

Nodes can be added with a guaranteed alignment. If needed padding will be inserted after the previous node to obtain this alignment.