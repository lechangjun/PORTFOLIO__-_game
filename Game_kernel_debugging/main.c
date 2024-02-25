#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define PROC_NAME "net_debug"

static struct proc_dir_entry* net_debug_entry;

// 파일에서 데이터를 읽는 함수
static ssize_t net_debug_read(struct file* file, char __user* ubuf, size_t count, loff_t* ppos) 
{
    // 실제로 데이터를 읽는 코드가 빠져 있으므로, 여기에 읽기 로직을 추가해야 함
    return 0; // 현재는 빈 데이터를 반환하므로 디버깅 목적으로만 사용 가능
}

// 파일에 데이터를 쓰는 함수
static ssize_t net_debug_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos) 
{
    // 실제로 데이터를 쓰는 코드가 빠져 있으므로, 여기에 쓰기 로직을 추가해야 함
    return count; // 현재는 받은 데이터의 길이를 그대로 반환하므로 디버깅 목적으로만 사용 가능
}

static const struct file_operations net_debug_fops = 
{
    .owner = THIS_MODULE,
    .read = net_debug_read,
    .write = net_debug_write,
};

static int __init net_debug_init(void) 
{
    // /proc 파일 시스템에 모듈 진입을 만듦
    net_debug_entry = proc_create(PROC_NAME, 0666, NULL, &net_debug_fops);
    if (!net_debug_entry) 
		{
        printk(KERN_ERR "Failed to create proc entry\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "net_debug module loaded\n");
    return 0;
}

static void __exit net_debug_exit(void) 
{
    if (net_debug_entry) 
		{
        proc_remove(net_debug_entry); // 모듈이 언로드될 때 /proc 진입을 제거
    }
    printk(KERN_INFO "net_debug module unloaded\n");
}

module_init(net_debug_init); // 모듈 초기화 함수 등록
module_exit(net_debug_exit); // 모듈 종료 함수 등록

MODULE_LICENSE("GPL"); // 모듈의 라이선스 지정
MODULE_AUTHOR("leechangjun"); // 모듈 작성자 지정
MODULE_DESCRIPTION("Network Debugging Module"); // 모듈 설명 지정