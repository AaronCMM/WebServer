#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include "../log/log.h"

// 升序链表的定时器（双向链表）

class util_timer;        // 声明
struct client_data      // 连接资源
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;                      //  任务的超时时间，使用 绝对时间
    void (*cb_func)(client_data *);     //  任务回调函数 (具体的实现在 main 函数中定义)
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

// 带有 头、尾节点 的双向升序链表
class sort_timer_lst
{
public:
    sort_timer_lst() : head(NULL), tail(NULL) {}
    ~sort_timer_lst()
    {  // 链表被销毁时，删除其中所有的定时器
        util_timer *tmp = head;
        while (tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    void add_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        if (!head)
        {
            head = tail = timer;
            return;
        }
        if (timer->expire < head->expire)     // 如果定时器的超时时间小于链表所有节点的超时时间，则将定时器插入链表的头节点
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer, head);         // 否则，调用 重载函数，寻找合适的插入位置，保证升序
    }
    void adjust_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        util_timer *tmp = timer->next;
        // 如果 被调整的目标定时器位于 链表尾部
        // 或者该定时器的值仍然小于 下一个定时器，则保持不变
        if (!tmp || (timer->expire < tmp->expire))  
        {
            return;
        }
        //被调整定时器是链表头结点，将定时器取出，重新插入
        if (timer == head)   
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, head);
        }
        //被调整定时器在内部，将定时器取出，重新插入
        else                      
        {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer, timer->next);
        }
    }

    void del_timer(util_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        if ((timer == head) && (timer == tail))
        {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        //被删除的定时器为头结点
        if (timer == head)
        {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        //被删除的定时器为尾结点
        if (timer == tail)
        {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    
    // 核心
    //定时任务处理函数
    void tick()
    {
        if (!head)
        {
            return;
        }

        LOG_INFO("%s", "timer tick");
        Log::get_instance()->flush();

        // 获得 系统的当前时间
        time_t cur = time(NULL);            
        util_timer *tmp = head;    
       // 遍历，从 头节点 开始处理定时器，直到遇到一个未超时的
        while (tmp)
        {
            //链表容器为升序排列
            //当前时间小于定时器的超时时间，后面的定时器也没有到期
            if (cur < tmp->expire)            
            {
                break;
            }
            
            //当前定时器到期，则调用回调函数，执行定时事件
            tmp->cb_func(tmp->user_data);       
            head = tmp->next;
            if (head)
            {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    //私有成员，被公有成员add_timer和adjust_time调用
   //主要用于调整链表内部结点
    void add_timer(util_timer *timer, util_timer *lst_head)     // 重载函数
    {
        util_timer *prev = lst_head;
        util_timer *tmp = prev->next;
// 遍历 lst_head 节点之后的部分链表，直到找到 一个超时时间大于目标定时器的节点，并将 目标节点 插入节点前面
        while (tmp)             
        {
            if (timer->expire < tmp->expire)
            {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        if (!tmp)  // 否则，插入到链表的尾节点
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }

private:
    util_timer *head;
    util_timer *tail;
};

#endif
