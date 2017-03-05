#define MAX_BUFFER 1024  ///< read buf size
unsigned char g_buf[MAX_BUFFER+1]; /// read buf 缓冲区
int g_bufLen;  /// 缓冲区内数据长度
static int DATASIZE = 64; /// hid读写数据长度，Report Length
IOHIDDeviceRef hidDev; /// hid设备
pthread_t pthreadID; ///input read thread
static pthread_mutex_t g_mutex; /// 读写锁
static pthread_cond_t g_cond;  /// 读写信号量
 
 
//根据pid，vid查找指定HID设备
bool GetDevice()
{
    const int vendorId = 0x1314;
    const int productId = 0x1011;
        //Matching HID Devices
    IOHIDManagerRef HIDManager = IOHIDManagerCreate(kCFAllocatorDefault,
                                 kIOHIDOptionsTypeNone);
    CFMutableDictionaryRef matchDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, 
                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if( matchDict)
    {
        CFNumberRef cfVendorId = CFNumberCreate(kCFAllocatorDefault, 
                                                        kCFNumberIntType, &vendorId);
        if(cfVendorId)
        {
            CFDictionaryAddValue(matchDict, CFSTR(kIOHIDVendorIDKey), cfVendorId);
            CFRelease(cfVendorId);
        }
        else
        {
            printf("CFNumberCreate cfVendorID failedn");
            return false;
        }
         
        CFNumberRef cfProductId = CFNumberCreate(kCFAllocatorDefault, 
                                                 kCFNumberIntType, &productId);
        if(cfProductId)
        {
            CFDictionaryAddValue(matchDict, CFSTR(kIOHIDProductIDKey), cfProductId);
            CFRelease(cfProductId);
        }
        else
        {
            printf("CFNumberCreate cfProductId failedn");
            return false;
        }
         
        IOHIDManagerSetDeviceMatching(HIDManager,  matchDict);
        CFRelease(matchDict);
        IOReturn kr = IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeSeizeDevice);
        if(kr == kIOReturnSuccess)
        {
            printf("okn");
        }
        else
        {
            printf("IOHIDManagerOpen error %xn", kr);
            return false;
        }
         
        CFSetRef devSetRef = IOHIDManagerCopyDevices(HIDManager);
        CFIndex devIndex, devCount = CFSetGetCount(devSetRef);
        printf("dev count : %dn", (int)devCount);
        if(devCount < = 0)
        {
            printf("get zero device,returnn");
            return false;
        }
        IOHIDDeviceRef * tIOHIDDeviceRefs = (IOHIDDeviceRef *)
                          malloc( sizeof( IOHIDDeviceRef ) * devCount );
        CFSetGetValues( devSetRef, ( const void ** )tIOHIDDeviceRefs );
        ///若devcount大于1，即匹配到了设备，则需要进一步筛选，此处直接选取第一个
        hidDev = tIOHIDDeviceRefs[0];
        return true;
    }
    else
    {
        return false;
    }
}
 
//HID设备的打开
bool Open()
{
    if(!GetDevice())
    {
        return false;
    }
        ///打开HID设备
    IOReturn kr = IOHIDDeviceOpen(hidDev, 0);
    if(kr == kIOReturnSuccess)
    {
        printf("open device successfullyn");
    }
    else
    {
        printf("open device failed kr = %Xn", kr);
        return false;
    }
    ///初始化缓冲区
    memset(g_buf, 0, MAX_BUFFER+1);
    g_buf[0] = '';
    g_bufLen = 0;
     
    ///建立读入数据线程
    pthread_attr_t attr;
    int returnVal;
    returnVal = pthread_attr_init(&attr);
    returnVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int threadError = pthread_create(&pthreadID, &attr, &Read_Thread, NULL);
    if(threadError != 0)
    {
        printf("create read thread error");
        return false;
    }
    wxMilliSleep(10);
    return true;
}
 
///向HID设备写入数据
int WriteData(const unsigned char *data, const int len)
{
    IOReturn kr = IOHIDDeviceSetReport(hidDev, kIOHIDReportTypeOutput, 0L, data, len);
    if(kr == kIOReturnSuccess)
    {
//        printf("write successfullyn");
        return len ;
    }
    else
        return -1;
}
 
///< input callback function 有数据读入时的回调函数，把数据保存到缓冲区
static void Handle_IOHIDDeviceIOHIDReportCallback(void *inContext, IOReturn inResult, void *inSender, 
                IOHIDReportType inType,uint32_t inReportID, uint8_t * inReport, CFIndex inReportLength)
{
    unsigned char *buffer = (unsigned char *)inReport;
    int size = inReportLength;
    if(size == DATASIZE)
    {
        int oldSize = g_bufLen;
        if(g_bufLen+size <= MAX_BUFFER)
        {
            pthread_mutex_lock(&g_mutex);
            memcpy(g_buf+g_bufLen, buffer, size);
            g_bufLen += size;
            g_buf[g_bufLen]='';
            pthread_mutex_unlock(&g_mutex);
            if(oldSize <= 0)
                pthread_cond_signal(&g_cond);
        }
        else
        {
            pthread_mutex_lock(&g_mutex);
            memcpy(g_buf, g_buf+size, g_bufLen-size);
            memcpy(g_buf+g_bufLen-size, buffer, size);
            g_buf[g_bufLen]='';
            pthread_mutex_unlock(&g_mutex);
        }
    }
}
 
 
///read thread 读设备的线程函数，注册回调函数
void *Read_Thread(void *arg)
{
    IOHIDDeviceScheduleWithRunLoop(hidDev, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    CFIndex reportSize = DATASIZE;
    uint8_t *report = (uint8_t *)malloc(reportSize);
    IOHIDDeviceRegisterInputReportCallback(hidDev, report, reportSize, Handle_IOHIDDeviceIOHIDReportCallback, NULL);
    CFRunLoopRun();
}
 
 
//从缓冲区读取数据
int ReadData(unsigned char *data, const int size)
{
    //缓冲区里没有数据可读时
    if(g_bufLen < size)
    {
        pthread_mutex_lock(&g_mutex);
        int ret = 0;
            struct timeval now;
        struct timespec tim;
        gettimeofday(&now, NULL);
        tim.tv_sec = now.tv_sec + 1;
        tim.tv_nsec = now.tv_usec * 1000;
        ///超时等待
        int retv = pthread_cond_timedwait(&g_cond, &g_mutex, &tim);
        if(g_bufLen >= size)
        {
            memcpy(data, g_buf, size);
            memcpy(g_buf, g_buf+size, g_bufLen-size);
            g_bufLen -= size;
            g_buf[g_bufLen]='';
            ret = size;
        }
           pthread_mutex_unlock(&g_mutex);
           return ret;
    }
    //缓冲区内有数据可读时
    else
    {
        pthread_mutex_lock(&g_mutex);
        memcpy(data, g_buf, size);
        memcpy(g_buf, g_buf+size, g_bufLen-size);
        g_bufLen -= size;
        g_buf[g_bufLen]='';
        pthread_mutex_unlock(&g_mutex);
        return size;
    }
}
 
//关闭HID设备
void CDeviceHid::Close()
{
    IOReturn kr = IOHIDDeviceClose(hidDev, 0);
    pthread_cancel(pthreadID);
    pthread_cond_destroy(&g_cond);
    pthread_mutex_destroy(&g_mutex);
}
