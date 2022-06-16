#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    gid_t gid;
    uid_t uid;
    gid = getegid();
    uid = geteuid();

    setregid(gid, gid);
    setreuid(uid, uid);

    system("sync; echo 3 > /proc/sys/vm/drop_caches");
}
