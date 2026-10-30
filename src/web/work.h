#ifndef ASYNCWORK_WORK_H
#define ASYNCWORK_WORK_H

struct AsyncWorkOps {
    char (*run)(void *work);
    void (*done)(void *work);
};

struct AsyncWork {
    const struct AsyncWorkOps *ops;
    unsigned started;
};

#endif
