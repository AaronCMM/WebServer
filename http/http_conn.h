#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
class http_conn
{
public:
    static const int FILENAME_LEN = 200;              // 文件名的最大长度
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD                         // HTTP 请求的方法
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE                     // 主状态机 的3种状态
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE                          // 服务器处理 http 请求的可能结果
    {
        NO_REQUEST,                         //  请求不完整，继续读取客户数据
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,                    // 服务器内部错误
        CLOSED_CONNECTION
    };
    enum LINE_STATUS                    // 从状态机
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr);
    void close_conn(bool real_close = true);
    void process();                            //  处理 客户请求
    bool read_once();                          //  非阻塞 读
    bool write();                             //   响应报文的写入函数 非阻塞
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);

private:
    void init();
    HTTP_CODE process_read();               // 从 m_read_buf读取，解析 HTTP 请求
    bool process_write(HTTP_CODE ret);        // 填充 HTTP 应答
    
    // 下面 这一组函数 被 process_read 调用 以分析 HTTP请求
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();            // 生成响应报文

    char *get_line() { return m_read_buf + m_start_line; };   // 用于将 指针向后偏移，指向未处理的字符
    LINE_STATUS parse_line();                   
    
    void unmap();

 //根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
//  所有 socket 上的事件都被注册到同一个 epoll内核事件表，所以 设置为 static类型
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;

private:
   // HTTP 连接的 socket 和对方的 socket 地址
    int m_sockfd;
    sockaddr_in m_address;
    
    // 读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    
    // 标志读缓存中 已经读入的客户数据的最后一个字节 的下一个位置
    int m_read_idx;
    // 当前正在分析的字符在 读缓冲中的位置
    int m_checked_idx;
    // 当前正在解析 的行的起始位置
    int m_start_line;
    
    // 写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    // 写缓冲区 待发送的字节数
    int m_write_idx;
    
    // 主状态机当前所在的状态
    CHECK_STATE m_check_state;
    // 请求方法
    METHOD m_method;
    
    //以下为解析请求报文中对应的6个变量
    char m_real_file[FILENAME_LEN];    // 客户请求的目标文件的完整路径。doc_root+ m_url,doc_root 为根目录。
    char *m_url;                 //  客户请求的目标文件名
    char *m_version;            
    char *m_host;
    int m_content_length;
    bool m_linger;             // HTTP 请求是否保持长连接
    
    char *m_file_address;      // 读取服务器上的文件地址
    // 目标文件的状态。 通过它 来判断 目标文件是否存在，是否为目录，是否可读，并获取文件的大小
    struct stat m_file_stat;
    
    
    struct iovec m_iv[2];  // 使用 writev 执行写操作  指向一个缓冲区
    int m_iv_count;         // 被写内存块的数量
    
    int cgi;        
    char *m_string; //存储请求头数据
    
    int bytes_to_send;
    int bytes_have_send;
};

#endif
