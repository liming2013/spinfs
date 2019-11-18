#include "spinfs.h"
#include "spi_flash.h"
//#include <stdio.h>

/*
        root_inode.magic1 = ;
        root_inode.name = ;
        root_inode.inode_num = ;
        root_inode.uid = ;
        root_inode.gid = ;
        root_inode.mode = ;
        root_inode.flags = ;
        root_inode.ctime = ;
        root_inode.mtime = ;
        root_inode.parent_inode = ;
        root_inode.version = ;
        root_inode.magic2 = ;
        root_inode.data_size = ;
*/

/*
 * Global variables
 */
uint32_t head, tail;
uint32_t ht_slot;       /* determine position of newest head and tail in Sec Regs*/
uint32_t inode_table_size;  // = array size - 1

/*
 * Update head and tail by pair to SEC_REG 1 and 2
 * Need to identify current head_tail_addr
 */
int update_head_tail(uint32_t head_n, uint32_t tail_n)
{
        return 0;
}

void print_node_info(struct spinfs_raw_inode *ri)
{
        int node_size =  sizeof(*ri) + ri->data_size;
        /*
         * There might a bug here when printing %s with ri->name when ri->name
         * is 32-bit length and there is no NULL character
         * Solution: use precision %.32s
         */
        printf("Size of i-node %d, %.*s is: %d + %d = %d\n"
                , ri->inode_num, MAX_NAME_LEN, ri->name, sizeof(*ri), ri->data_size, node_size);
        print_buffer((unsigned char *)ri, node_size);
        printf("\n");
}

void print_inode_table(struct inode_table_entry *it)
{

        printf("    I-node    |    Address     |   Version    \n");
        for (int i = 1; i <= inode_table_size; i++) {
                printf("     %4d         0x%06x           %4d   \n", i, it[i].physical_addr, it[i].version);
        }
}

void ls_file(struct spinfs_raw_inode *inode)
{
        printf("Output:\n");
        printf("%.*s\n", MAX_NAME_LEN, inode->name);
}

void print_directory(struct spinfs_raw_inode *inode)
{
        // TODO check inode is a directory

        int dir_file_count = inode->data_size / sizeof(struct dir_entry);
        printf("Directory %.*s", MAX_NAME_LEN, inode->name);
        if (dir_file_count == 0) {
                printf(" is empty.\n");
                return;
        }
        printf(" has %d file(s).\n", dir_file_count);
        printf("    Entry index    |    File name     |   I-node number    \n");
        for (int i = 0; i < dir_file_count; i++) {
                printf("     %4d                   %.*s                %4d   \n",
                                i, MAX_NAME_LEN,
                                ((struct dir_entry *)inode->data)[i].name,
                                ((struct dir_entry*)inode->data)[i].inode_num);
        }
}

uint32_t find_file_in_dir(struct spinfs_raw_inode *inode, char *filename)
{
        // TODO check inode is a directory
        if (S_ISREG(inode->mode)) {
                printf("%.*s is a regular file.\n", MAX_NAME_LEN, inode->name);
                return 0;
        }
        else if (S_ISDIR(inode->mode)) {
                printf("%.*s is a directory.\n", MAX_NAME_LEN, inode->name);
        }

        int dir_file_count = inode->data_size / sizeof(struct dir_entry);
        for (int i = 0; i < dir_file_count; i++) {
                printf("Index %d, %.*s, target %s :", i, MAX_NAME_LEN, ((struct dir_entry *)inode->data)[i].name, filename);
                if (strncmp(filename, ((struct dir_entry *)inode->data)[i].name, MAX_NAME_LEN) == 0) {
                        printf("matched.\n");
                        printf("\n");
                        return ((struct dir_entry *)inode->data)[i].inode_num;
                } else {
                        printf("unmatched.\n");
                }
        }
        printf("\n");
        return 0;
}

void spinfs_scan_fs(uint32_t head, uint32_t tail)
{
}

/*
 * Return inode structure from a physical address
 */
void spinfs_get_inode_at_addr(struct spinfs_raw_inode *s, uint32_t addr)
{
        //struct spinfs_raw_inode holder;
        s = realloc(s, sizeof(*s));
}

/*
struct spinfs_raw_inode *createInode(unsigned char *d_buf, uint32_t d_size)
{
        struct spinfs_raw_inode *i = malloc(sizeof(*i) + d_size);         //need to free manually

        strncpy(i->name, "This_is_the_name_of_root_directory", 32);
        i->inode_num = 255;
        i->parent_inode_num = 0;
        i->data_size = 0;
        i->version = 1;
        //code to memcpy d_buf to data member
        if (d_buf != NULL) {
                memcpy(i->data, d_buf, d_size);
        }
        return i;
}
*/

/*
 * Copy a file to the end of flash
 */
/*
void spinfs_cp(FILE *f)
{

}
/**/

#if 0
int main()
{
        //spi_init();
        printf("Size of struct: %d\n", sizeof(struct test_str));
        struct test_str arr[2];
        printf("Addr of arr: %p\n", arr);
        printf("Addr of arr + 1: %p\n", arr + 1);
        printf("Addr of arr->m: %p\n", &(arr->m));
        printf("Addr of arr[0]: %p\n", &arr[0]);
        printf("Addr of arr[1]: %p\n", &arr[1]);
        printf("Addr of arr[0].m: %p\n", &(arr[0].m));
        struct test_str *p = malloc(2*(sizeof(*p) + sizeof(int)));
        printf("Addr of p: %p\n", p);
        printf("Addr of p + 1: %p\n", p + 1);
        printf("Size of object: %d\n", sizeof(*p) + sizeof(int));

        free(p);
        return 0;
}
#endif

#if 0
int main()
{
        head = 0x000000;
        tail = 0x000000;
        struct spinfs_raw_inode *root_inode = createInode(NULL, 0);
        //root_inode->name[0] = '/';
        strncpy(root_inode->name, "This_is_the_name_of_root_directory", 32);
        root_inode->inode_num = 255;
        root_inode->parent_inode_num = 0;
        root_inode->data_size = 0;
        root_inode->version = 1;

        printf("Current i-node size: %d bytes.\n", sizeof(*root_inode));
        print_buffer((unsigned char*)root_inode, sizeof(*root_inode));

        int fd_spi = spi_init();
        //spi_write_data(0x020000, (unsigned char*)root_inode, sizeof(*root_inode));
        spi_close(fd_spi);
        free(root_inode);
        return 0;
}
#endif
