#include <mpc_common_flags.h>

struct mpc_common_flags * mpc_common_get_flags()
{
        static struct mpc_common_flags flags = { 0 };
        return &flags;
}