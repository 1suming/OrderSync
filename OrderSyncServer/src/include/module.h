#ifndef __MODULE_INTERFACE_H__
#define __MODULE_INTERFACE_H__

#include <core.def>

__INTERFACE__ IModule
{
public:
    IModule (void)
    {
    }

    virtual ~IModule (void)
    {
    }

    virtual int open (int argc, char** argv) = 0;
    virtual int close (void) = 0;

    virtual int run (void* parameter) = 0;
    virtual int exception (void) = 0;
};

#define MODULE_INSTANCE_EXPORT(ModuleClass, ModuleName, ModuleVersion) \
        ModuleClass* g_##ModuleName##_##ModuleVersion = NULL; \
        extern "C" IModule* get_module_instance(void) \
        { \
            if (NULL == g_##ModuleName##_##ModuleVersion) \
            { \
                try \
                { \
                    g_##ModuleName##_##ModuleVersion = new ModuleClass; \
                } \
                catch (...) \
                { \
                    return NULL; \
                } \
            } \
            return g_##ModuleName##_##ModuleVersion; \
        }

#define MODULE_DESTROY_EXPORT(ModuleClass, ModuleName, ModuleVersion) \
        extern "C" void destroy_module(void) \
        { \
            if (NULL != g_##ModuleName##_##ModuleVersion) \
            { \
                delete g_##ModuleName##_##ModuleVersion; \
                g_##ModuleName##_##ModuleVersion = NULL; \
            } \
            return; \
        }


#endif //__MODULE_INTERFACE_H__
