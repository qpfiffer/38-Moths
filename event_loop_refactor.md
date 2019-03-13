Whats currently happening?

1. Acceptor thread accepts new connections, throws them onto a queue
2. Workers pop items off the queue, figure out what to do, call handlers and
   write data out to the waiting socket on the other end.
3. The worker then closes the socket.

Why is this bad?

Workers are currently blocked until they send the WHOLE response to the client.
This is bad because you could have a slow client, or a big file, or pretty much
anything. I think they way I want to deal with this is to add another queue that
additional workers will pop objects off of. This will be the sending queue or
something.

So the way I want this to work:

1. Acceptor thread accepts new connections, throws them onto the 'accepted but
   not handled' queue
2. A handful of workers pop items off this queue and handle responses
3. Responses are queued up in memory or whatever (could be a bad idea for larger
   responses) and added to the 'handled but not sent' queue
4. Another worker simply consumes from this queue, sends about 4k bytes
   (configurable?) from the response, puts the item back on the queue and goes
   to the next one.
