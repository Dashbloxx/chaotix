#pragma once

// multiboot.h
typedef struct multiboot_info multiboot_info_t;

// system.h
typedef struct registers registers;

// memory.h
typedef struct page_directory page_directory;
typedef struct page_table page_table;
typedef union page_table_entry page_table_entry;

// process.h
struct process;

// fs/fs.h
typedef struct file_description file_description;

// socket.h
typedef struct unix_socket unix_socket;

// growable_buf.h
typedef struct growable_buf growable_buf;

// api/time.h
struct timespec;
