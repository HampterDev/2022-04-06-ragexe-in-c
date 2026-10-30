#include "core/abstract_method.h"
#include "core/fatal.h"

void abstract_method_called(void)
{
    fatal_error("abstract method called");
}
