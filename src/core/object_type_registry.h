#ifndef CORE_OBJECT_TYPE_REGISTRY_H
#define CORE_OBJECT_TYPE_REGISTRY_H

#include "core/object_type.h"

/* The composition point of the object types (core/object_type.h): every row table,
   installed by client_install_hooks(). */
extern const struct ObjectTypeTable g_objectTypes;

/* The tables it lists, each in the *_object_types.c of the directory of its classes. */
extern const struct ObjectTypeRows g_controlsObjectTypes;
extern const struct ObjectTypeRows g_itemsObjectTypes;
extern const struct ObjectTypeRows g_characterObjectTypes;
extern const struct ObjectTypeRows g_questsObjectTypes;
extern const struct ObjectTypeRows g_socialObjectTypes;
extern const struct ObjectTypeRows g_shopsObjectTypes;
extern const struct ObjectTypeRows g_chatObjectTypes;
extern const struct ObjectTypeRows g_actorObjectTypes;
extern const struct ObjectTypeRows g_resourceObjectTypes;
extern const struct ObjectTypeRows g_mailObjectTypes;
extern const struct ObjectTypeRows g_hotkeyObjectTypes;
extern const struct ObjectTypeRows g_skillObjectTypes;
extern const struct ObjectTypeRows g_renderUiObjectTypes;
extern const struct ObjectTypeRows g_mapObjectTypes;
extern const struct ObjectTypeRows g_worldmapObjectTypes;

#endif
