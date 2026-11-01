#include "contents/agency_internal.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <mmsystem.h>

int agency_transport_open(void)
{ return connection_is_open(rag_connection_transport(rag_connection_instance())); }
int agency_transport_send(const void *packet, size_t size)
{ return rag_connection_send_packet(rag_connection_instance(), (int)size, packet); }
uint32_t agency_clock(void) { return timeGetTime(); }
