#include "hv/hloop.h"

void on_close(hio_t* io){}

void on_recv(hio_t* io, void* buf, int readbytes) {
	hio_write(io, buf, readbytes);
}

void on_accept(hio_t* io) {
	hio_setcb_close(io, on_close);
	hio_setcb_read(io, on_recv);
	hio_read(io);
}

int main2()
{
	hloop_t* loop = hloop_new(0);
	hio_t* listenio = hloop_create_tcp_server(loop, "0.0.0.0", 552, on_accept);
	if (listenio == NULL) {
		return -20;
	}
	hloop_run(loop);
	hloop_free(&loop);
	return 0;
}