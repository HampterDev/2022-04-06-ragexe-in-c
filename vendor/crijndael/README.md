# CRijndael

`Rijndael.cpp` and `Rijndael.h` are George Anescu's public-domain CRijndael class; `rijndael_api.cpp`
is the client's C entry point (`core/rijndael_api.h`).

Local patch: errors are thrown as `std::runtime_error` instead of `std::exception` built from a
message, which only Microsoft's library accepts. The client catches every exception the same way.
