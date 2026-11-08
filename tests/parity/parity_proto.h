#ifndef PARITY_PROTO_H
#define PARITY_PROTO_H



enum {
    
    PARITY_HELLO = 1,       
    PARITY_SYNC = 2,        
    PARITY_EV_CONNECT = 10, 
    PARITY_EV_SEND = 11,    
    PARITY_EV_CLOSE = 12,   
    PARITY_EV_LOG = 13,     
    PARITY_EV_DIALOG = 14,  

    
    PARITY_CMD_DELIVER = 20,    
    PARITY_CMD_POST = 21,       
    PARITY_CMD_KEY = 22,        
    PARITY_CMD_CURSOR = 23,     
    PARITY_CMD_CAPTURE = 24,    
    PARITY_CMD_STATE = 25,      
    PARITY_CMD_QUIT = 26,       
    PARITY_CMD_PEER_CLOSE = 27, 
    PARITY_CMD_CONNECTED = 28,  
    PARITY_CMD_REGDUMP = 29,    
    PARITY_CMD_CALL = 30,       
    PARITY_CMD_PEER_RESET = 31, 
    PARITY_CMD_GO = 100
};

enum {
    PARITY_SYNC_FRAME = 1,   
    PARITY_SYNC_CONNECT = 2, 
    PARITY_SYNC_NETWAIT = 3  
};

enum {
    PARITY_TARGET_MAIN = 0, 
    PARITY_TARGET_FOCUS = 1 
};

#endif
