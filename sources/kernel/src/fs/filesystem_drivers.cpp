#include <fs/filesystem.h>

// includujeme prislusne drivery
#include <fs/drivers/gpio_fs.h>
#include <fs/drivers/uart_fs.h>
#include <fs/drivers/i2c_fs.h>
#include <fs/drivers/semaphore_fs.h>
#include <fs/drivers/mutex_fs.h>
#include <fs/drivers/condvar_fs.h>
#include <fs/drivers/pipe_fs.h>

// pole driveru - tady uvedeme vsechny, ktere jsou v systemu dostupne a ktere je zadouci pro tuto instanci naseho OS pripojit
const CFilesystem::TFS_Driver CFilesystem::gFS_Drivers[] = {
    // "skutecna" zarizeni
    { "GPIO_FS", "DEV:gpio", &fsGPIO_FS_Driver },
    { "UART_FS", "DEV:uart", &fsUART_FS_Driver },
    { "I2C_FS", "DEV:i2c", &fsI2C_FS_Driver },

    // virtualni zarizeni
    { "Mutex", "SYS:mtx", &fsMutex_FS_Driver },
    { "Semaphore", "SYS:sem", &fsSemaphore_FS_Driver },
    { "CondVar", "SYS:cv", &fsCond_Var_FS_Driver },
    { "Pipe", "SYS:pipe", &fsPipe_FS_Driver },
};

// pocet FS driveru - je staticky spocitan z velikosti vyse uvedeneho pole
const uint32_t CFilesystem::gFS_Drivers_Count = sizeof(CFilesystem::gFS_Drivers) / sizeof(CFilesystem::TFS_Driver);
