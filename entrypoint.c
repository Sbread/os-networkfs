#include "entrypoint.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Osipov Daniil");
MODULE_VERSION("0.01");

// const char* my_token = "101fcc9c-b3fa-415c-952e-a29ca94db5d4";

struct file_system_type networkfs_fs_type = {.name = "networkfs",
                                             .mount = networkfs_mount,
                                             .kill_sb = networkfs_kill_sb};

struct file_operations networkfs_dir_ops = {
    .iterate = networkfs_iterate,
    .read = networkfs_read,
    .write = networkfs_write,
};

struct inode_operations networkfs_inode_ops = {
    .lookup = networkfs_lookup,
    .create = networkfs_create,
    .unlink = networkfs_unlink,
    .mkdir = networkfs_mkdir,
    .rmdir = networkfs_rmdir,
    .link = networkfs_link,
};

int networkfs_iterate(struct file *filp, struct dir_context *ctx) {
  struct inode *inode;
  unsigned long offset;
  struct entries call_storage;
  memset(&call_storage, 0, sizeof(call_storage));
  inode = filp->f_inode;
  offset = ctx->pos;
  ino_t ino = inode->i_ino;
  char ino_str[20];
  sprintf(ino_str, "%lu", ino);
  int64_t ret = networkfs_http_call(
      GET_TOKEN_FROM_INODE(inode), "list", (char *)&call_storage,
      sizeof(struct entries), 1, "inode", ino_str);
  if (ret != 0) {
    printk(KERN_INFO "iterate: networkfs_http_call error");
    return ret;
  }
  while (offset < call_storage.entries_count) {
    struct entry ent = call_storage.entries[offset];
    dir_emit(ctx, ent.name, strlen(ent.name), ent.ino, ent.entry_type);
    ctx->pos++;
    offset++;
  }
  return offset;
}

char *name_convertor(char *name, size_t len) {
  char tmp[3];
  char *ret = kmalloc(3 * len, GFP_KERNEL);
  if (!ret) {
    return ret;
  }
  for (size_t i = 0; i < len; ++i) {
    sprintf(tmp, "%%%02x", (int)name[i]);
    strcat(ret, tmp);
  }
  return ret;
}

struct dentry *networkfs_lookup(struct inode *parent_inode,
                                struct dentry *child_dentry,
                                unsigned int flag) {
  ino_t root = parent_inode->i_ino;
  struct inode *inode;
  struct entry_info ent_info;
  memset(&ent_info, 0, sizeof(ent_info));
  const char *name = child_dentry->d_name.name;
  char root_str[20];
  sprintf(root_str, "%lu", root);
  char *name_cnv = name_convertor(name, strlen(name));
  if (!name_cnv) {
    kfree(name_cnv);
    printk(KERN_INFO "allocated error");
    return NULL;
  }
  int64_t ret = networkfs_http_call(
      GET_TOKEN_FROM_INODE(parent_inode), "lookup", (char *)&ent_info,
      sizeof(struct entry_info), 2, "parent", root_str, "name", name_cnv);
  if (ret != 0) {
    kfree(name_cnv);
    printk(KERN_INFO "lookup_call_error");
    return NULL;
  }
  inode = networkfs_get_inode(
      parent_inode->i_sb, NULL,
      (ent_info.entry_type == DT_DIR ? S_IFDIR : S_IFREG), ent_info.ino);
  d_add(child_dentry, inode);
  kfree(name_cnv);
  return child_dentry;
}

int networkfs_create(struct user_namespace *u_nmspc, struct inode *parent_inode,
                     struct dentry *child_dentry, umode_t mode, bool b) {
  const char *name = child_dentry->d_name.name;
  char *name_cnv = name_convertor(name, strlen(name));
  if (!name_cnv) {
    kfree(name_cnv);
    printk(KERN_INFO "allocated error");
    return NULL;
  }
  ino_t root = parent_inode->i_ino;
  char root_str[20];
  sprintf(root_str, "%lu", root);
  ino_t ino;
  struct inode *inode;
  int64_t ret = networkfs_http_call(
      GET_TOKEN_FROM_INODE(parent_inode), "create", (char *)&ino, sizeof(ino_t),
      3, "parent", root_str, "name", name_cnv, "type", "file");
  if (ret != 0) {
    kfree(name_cnv);
    printk(KERN_INFO "create_call_error");
    return ret;
  }
  inode = networkfs_get_inode(parent_inode->i_sb, NULL, S_IFREG | 0x777, ino);
  if (inode != NULL) {
    d_add(child_dentry, inode);
  }
  kfree(name_cnv);
  return ret;
}

int networkfs_unlink(struct inode *parent_inode, struct dentry *child_dentry) {
  const char *name = child_dentry->d_name.name;
  char *name_cnv = name_convertor(name, strlen(name));
  if (!name_cnv) {
    kfree(name_cnv);
    printk(KERN_INFO "allocated error");
    return NULL;
  }
  ino_t root = parent_inode->i_ino;
  char root_str[20];
  sprintf(root_str, "%lu", root);
  int64_t ret =
      networkfs_http_call(GET_TOKEN_FROM_INODE(parent_inode), "unlink", NULL, 0,
                          2, "parent", root_str, "name", name_cnv);
  kfree(name_cnv);
  return ret;
}

int networkfs_mkdir(struct user_namespace *u_nmspc, struct inode *parent_inode,
                    struct dentry *child_dentry, umode_t mode) {
  const char *name = child_dentry->d_name.name;
  char *name_cnv = name_convertor(name, strlen(name));
  if (!name_cnv) {
    kfree(name_cnv);
    printk(KERN_INFO "allocated error");
    return NULL;
  }
  ino_t root = parent_inode->i_ino;
  char root_str[20];
  sprintf(root_str, "%lu", root);
  ino_t ino;
  struct inode *inode;
  int64_t ret = networkfs_http_call(
      GET_TOKEN_FROM_INODE(parent_inode), "create", (char *)&ino, sizeof(ino_t),
      3, "parent", root_str, "name", name_cnv, "type", "directory");
  if (ret != 0) {
    kfree(name_cnv);
    printk(KERN_INFO "create_call_error");
    return ret;
  }
  inode = networkfs_get_inode(parent_inode->i_sb, NULL, S_IFDIR | 0x777, ino);
  if (inode != NULL) {
    d_add(child_dentry, inode);
  }
  kfree(name_cnv);
  return ret;
}

int networkfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry) {
  const char *name = child_dentry->d_name.name;
  char *name_cnv = name_convertor(name, strlen(name));
  if (!name_cnv) {
    kfree(name_cnv);
    printk(KERN_INFO "allocated error");
    return NULL;
  }
  ino_t root = parent_inode->i_ino;
  char root_str[20];
  sprintf(root_str, "%lu", root);
  int64_t ret =
      networkfs_http_call(GET_TOKEN_FROM_INODE(parent_inode), "rmdir", NULL, 0,
                          2, "parent", root_str, "name", name_cnv);
  kfree(name_cnv);
  return ret;
}

ssize_t networkfs_read(struct file *filp, char *buffer, size_t len,
                       loff_t *offset) {
  struct inode *inode = filp->f_inode;
  struct content cont;
  memset(&cont, 0, sizeof(cont));
  char ino_str[20];
  sprintf(ino_str, "%lu", inode->i_ino);
  int64_t ret =
      networkfs_http_call(GET_TOKEN_FROM_INODE(inode), "read", (char *)&cont,
                          sizeof(cont), 1, "inode", ino_str);
  if (ret != 0) {
    printk(KERN_INFO "read_call_error");
    return ret;
  }
  while (*offset < cont.content_length) {
    put_user(cont.content[*offset], buffer + *offset);
    ++ret;
    (*offset)++;
  }
  return ret;
}

ssize_t networkfs_write(struct file *filp, const char *buffer, size_t len,
                        loff_t *offset) {
  struct inode *inode = filp->f_inode;
  char ino_str[20];
  sprintf(ino_str, "%lu", inode->i_ino);
  char content[len];
  ssize_t cnt = 0;
  while (*offset < len) {
    get_user(content[*offset], buffer + *offset);
    cnt++;
    (*offset)++;
  }
  char *content_conv = name_convertor(content, len);
  int64_t ret =
      networkfs_http_call(GET_TOKEN_FROM_INODE(inode), "write", NULL, 0, 2,
                          "inode", ino_str, "content", content_conv);
  kfree(content_conv);
  return ret == 0 ? cnt : ret;
}

int networkfs_link(struct dentry *old_dentry, struct inode *parent_dir,
                   struct dentry *new_dentry) {
  struct inode *inode = old_dentry->d_inode;
  char *name = new_dentry->d_name.name;
  char parent_str[20];
  sprintf(parent_str, "%lu", parent_dir->i_ino);
  char source_str[20];
  sprintf(source_str, "%lu", inode->i_ino);
  char *name_cnv = name_convertor(name, strlen(name));
  if (!name_cnv) {
    kfree(name_cnv);
    return 0;
  }
  int64_t ret = networkfs_http_call(GET_TOKEN_FROM_INODE(inode), "link", NULL,
                                    0, 3, "source", source_str, "parent",
                                    parent_str, "name", name_cnv);
  kfree(name_cnv);
  return ret;
}

void networkfs_kill_sb(struct super_block *sb) {
  kfree(sb->s_fs_info);
  printk(KERN_INFO
         "networkfs super block is destroyed. Unmount successfully.\n");
}

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *dir, umode_t mode,
                                  int i_ino) {
  struct inode *inode;
  inode = new_inode(sb);
  if (inode != NULL) {
    inode->i_ino = i_ino;
    inode->i_op = &networkfs_inode_ops;
    inode->i_fop = &networkfs_dir_ops;
    inode_init_owner(&init_user_ns, inode, dir, mode);
  }
  return inode;
}

int networkfs_fill_super(struct super_block *sb, void *data, int silent) {
  struct inode *inode;
  inode = networkfs_get_inode(sb, NULL, S_IFDIR | 0x777, 1000);
  sb->s_root = d_make_root(inode);
  if (sb->s_root == NULL) {
    return -ENOMEM;
  }
  printk(KERN_INFO "return 0\n");
  return 0;
}

struct dentry *networkfs_mount(struct file_system_type *fs_type, int flags,
                               const char *token, void *data) {
  struct dentry *ret;
  char *token_copy;
  ret = mount_nodev(fs_type, flags, data, networkfs_fill_super);
  if (ret == NULL) {
    printk(KERN_ERR "Can't mount file system");
    goto err;
  }
  ret->d_sb->s_fs_info = NULL;
  token_copy = kmalloc(strlen(token), GFP_KERNEL);
  if (!token_copy) {
    printk(KERN_INFO "allocate error");
    goto err;
  }
  strcpy(token_copy, token);
  ret->d_sb->s_fs_info = (void *)token_copy;
  printk(KERN_INFO "Mounted successfuly");
  return ret;

err:
  return token_copy ? ret : 0;
}

int networkfs_init(void) {
  // printk(KERN_INFO "Hello, World!\n");
  register_filesystem(&networkfs_fs_type);
  return 0;
}

void networkfs_exit(void) {
  // printk(KERN_INFO "Goodbye!\n");
  unregister_filesystem(&networkfs_fs_type);
}

module_init(networkfs_init);
module_exit(networkfs_exit);
