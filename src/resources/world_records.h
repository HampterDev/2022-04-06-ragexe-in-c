#ifndef RESOURCES_WORLD_RECORDS_H
#define RESOURCES_WORLD_RECORDS_H

struct RswModel {
    char name[41], fileName[81], nodeName[81];
    float pos[3], rot[3], scale[3];
    int animType, blockType;
    float animSpeed;
    unsigned char flag;
};

struct RswEffect {
    char name[81];
    float pos[3];
    int id;
    float emitSpeed, param[4];
};

struct RswSound {
    char name[81], waveName[81];
    float pos[3], volume;
    int width, height;
    float range, cycle;
};

#define RSW_TREE_DEPTH 5
#define RSW_TREE_NODE_COUNT 1365
struct RswBounds {
    float max[3], min[3], half[3], centre[3];
};

#endif
