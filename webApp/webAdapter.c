/**@mainpage  gw_web管理程序
* <table>
* <tr><th>Project  <td>gw_web 
* <tr><th>Author   <td>liqing 
* <tr><th>Source   <td>gitlab
* </table>
* @section   项目详细描述
* Web管理主要用于嵌入式设备状态监测，实现功能包括界面显示，文件下载，后台相关功能进行权限管理。设备上可以查看的设备状态包括基带相关寄存器值，射频以及中频相关寄存器值，信号强度（RSSI）以及信道的频谱和时域表示。 文件存储功能包括RSSI和信道数据的文件存储
。
*
* @section   功能描述  
* -# 本工程基于arm嵌入式环境开发，前端基于node js开发
* -# 后端提供数据和操作管理，前端负责展示数据和提供用户交互接口
* -# 前后端接口交互基于进程间通信
* 
* @section   用法描述 
* -# 网线连接设备，确认连接网口IP信息，例如192.168.10.77
* -# 浏览器中输入 http://192.168.10.77:32000/，进入设备用户登录界面
* -# 用户登录界面，输入例如：用户名：admin 密码： 123456
* 
* @section   程序更新 
* <table>
* <tr><th>Date        <th>H_Version    <th>Author    <th>Description  </tr>
* <tr><td>2019/11/05  <td>1.0    <td>liqing  <td>创建初始版本 </tr>
* <tr><td>2019/11/18  <td>1.0    <td>liqing  <td>
* -# 增加显示寄存器数值，rssi数值功能；
* -# 新增一次请求对应一个连接
* </tr>
* <tr><td>2019/11/21  <td>1.0    <td>liqing  <td>添加MD5sum用于后端在线模拟前端输入 </tr>
* <tr><td>2019/11/27  <td>1.0    <td>liqing  <td>
* -# 后端网络层添加user_node_list管理连接用户；
* -# 不同数据源模块添加如rssi_node_list用于管理访问该模块用户
* -# 后端支持多用户访问
</tr>
* <tr><td>2019/12/02  <td>1.0    <td>liqing  <td>添加保存rssi文件 </tr>
* <tr><td>2019/12/12  <td>1.0    <td>liqing  <td>
* -# 添加信道估计显示数据处理，保存
* -# 添加星座图显示数据处理  
</tr>
* <tr><td>2020/01/07  <td>1.0    <td>liqing  <td>
* -# 添加射频信息，网络统计信息显示
* -# 添加控制设备dac，配置ip，控制射频等写设备接口 
</tr>
* </table>
**********************************************************************************
*/

/**@file  main.c
* @brief       主函数文件
* @details  主要包含各个模块初始化，事件调度循环初始化，main函数入口
* @author      liqing
* @date        2019-11-07
* @version     V1.0
* @copyright    Copyright (c) 2019-2020  成都吉纬科技有限公司
**********************************************************************************
* @attention
* 硬件平台:GW45 GW50
* @par 修改日志:
* <table>
* <tr><th>Date        <th>Version  <th>Author    <th>Description
* <tr><td>2019/11/07  <td>1.0      <td>liqing  <td>init version
* </table>
*
**********************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "zlog.h"

#include "msg_queue.h"
#include "event_process.h"
#include "mosq_broker.h"
#include "dma_handler.h"
#include "ThreadPool.h"
#include "event_timer.h"
#include "web_common.h"
#include "cmd_line.h"

int debug_switch = 0; // 0 : no zlog file 
zlog_category_t * initLog(const char* path, char* app_name){
	int rc;
	zlog_category_t *zlog_handler = NULL;

	rc = zlog_init(path);

	if (rc) {
		printf("init initLog failed\n");
		return NULL;
	}

	zlog_handler = zlog_get_category(app_name);

	if (!zlog_handler) {
		printf("get cat fail\n");

		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeLog(){
	zlog_fini();
}

void info_zlog(zlog_category_t *zlog_handler, const char *format, ...)
{
	if(debug_switch == 0){
		return;
	}
	char log_buf[1024] = { 0 };
	va_list args;
	va_start(args, format);
	vsprintf(log_buf, format, args);
	va_end(args);
	zlog_info(zlog_handler, log_buf);
}

void check_assert(){
	assert(sizeof (uint32_t) == 4);
	printf("uint32_t : %d , uint64_t : %d \n", sizeof(uint32_t),sizeof(u_int64_t)); 
	printf("gw_web version built time is:[%s  %s]\n",__DATE__,__TIME__);
}

int main(int argc,char** argv)
{

	//check_assert();

	g_args_para g_args = {
		.prog_name = NULL,
		.conf_file = NULL,
		.log_file  = NULL,
		.debug_switch = 0
	}; 
	int ret = parse_cmd_line(argc, argv, &g_args);
	if(-EINVAL == ret){
		fprintf (stdout, "parse_cmd_line error : %s \n", g_args.prog_name);
        return 0;
	}else if(-EPERM == ret){
		return 0;
	}
	fprintf (stdout, "cmd line : %s -l %s\n", g_args.prog_name, g_args.log_file);
	
	debug_switch = g_args.debug_switch;

	zlog_category_t *zlog_handler = initLog(g_args.log_file,g_args.prog_name);

	zlog_info(zlog_handler,"******************** start webAdapter process ********************************\n");

	zlog_info(zlog_handler,"this version built time is:[%s  %s]\n",__DATE__,__TIME__);
	
	/* msg_queue */
	g_msg_queue_para* g_msg_queue = createMsgQueue();
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	//zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	/* reg dev */
	g_RegDev_para* g_RegDev = NULL;
	int state = initRegdev(&g_RegDev, 0x43C20000, zlog_handler);
	if(state != 0 ){
		zlog_info(zlog_handler,"initRegdev create failed !");
		return 0;
	}

	/* Timer handler */
	event_timer_t* g_timer = NULL;
    state = ngx_event_timer_init(&g_timer);
    if(state != 0){
        zlog_info(zlog_handler, "ngx_event_timer_init error , stat = %d \n", state);
        return 0;
    }

	/* ThreadPool handler */
	ThreadPool* g_threadpool = NULL;
	createThreadPool(64, 20, &g_threadpool);

	/* server thread */
	g_server_para* g_server = NULL;
	state = CreateServerThread(&g_server, g_threadpool, g_msg_queue, g_timer, zlog_handler);
	if(state == -1 || g_server == NULL){
		zlog_info(zlog_handler,"No server thread created \n");
		return 0;
	}

	/* broker handler */
	g_broker_para* g_broker = NULL;
	state = createBroker(g_args.prog_name, &g_broker, g_server, g_RegDev, zlog_handler);
	if(state != 0 || g_server == NULL){
		zlog_info(zlog_handler,"No Broker created \n");
		return 0;
	}

	/* dma handler */
	g_dma_para* g_dma = NULL;
	state = create_dma_handler(&g_dma, g_server, zlog_handler);
	if(state != 0){
		zlog_info(zlog_handler,"No dma handler created !\n");
		return 0;
	}

	eventLoop(g_server, g_broker, g_dma, g_msg_queue, g_threadpool, g_timer, zlog_handler);

	closeLog();
    return 0;
}

