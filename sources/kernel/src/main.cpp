#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/timer.h>
#include <interrupt_controller.h>

#include <memory/memmap.h>
#include <memory/kernel_heap.h>

#include <process/process_manager.h>

#include <fs/filesystem.h>

#include <stdstring.h>
#include <stdfile.h>

extern "C" void Timer_Callback()
{
	sProcessMgr.Schedule();
}

extern "C" unsigned char __init_task[];
extern "C" unsigned int __init_task_len;

extern "C" unsigned char __sos_task[];
extern "C" unsigned int __sos_task_len;

extern "C" unsigned char __logger_task[];
extern "C" unsigned int __logger_task_len;

extern "C" unsigned char __i2c_master_task[];
extern "C" unsigned int __i2c_master_task_len;

extern "C" unsigned char __i2c_slave_task[];
extern "C" unsigned int __i2c_slave_task_len;

extern "C" int _kernel_main(void)
{
	// inicializace souboroveho systemu
	sFilesystem.Initialize();

	sUART0.Open();
	sUART0.Set_Baud_Rate(NUART_Baud_Rate::BR_115200);
	sUART0.Set_Char_Length(NUART_Char_Length::Char_8);
	volatile unsigned int tim;
	char buf[10];
	for(int i = 3; i > 0; i--) {
		sUART0.Write(itoa(i, buf, 10));
		sUART0.Write("\r\n");
		for(tim = 0; tim < 5000000; tim++)
			;
	}
	sUART0.Close();

	// vytvoreni hlavniho systemoveho (idle) procesu
	sProcessMgr.Create_Process(__init_task, __init_task_len, true);

	// vytvoreni vsech tasku
	// TODO: presunuti do init procesu a nejake inicializacni sekce
	//sProcessMgr.Create_Process(__sos_task, __sos_task_len, false);
	sProcessMgr.Create_Process(__logger_task, __logger_task_len, false);
	sProcessMgr.Create_Process(__i2c_master_task, __i2c_master_task_len, false);
	sProcessMgr.Create_Process(__i2c_slave_task, __i2c_slave_task_len, false);

	// zatim zakazeme IRQ casovace
	sInterruptCtl.Disable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	// nastavime casovac - v callbacku se provadi planovani procesu
	sTimer.Enable(Timer_Callback, 0x80, NTimer_Prescaler::Prescaler_1);

	// povolime IRQ casovace
	sInterruptCtl.Enable_Basic_IRQ(hal::IRQ_Basic_Source::Timer);

	// povolime IRQ (nebudeme je maskovat) a od tohoto momentu je vse v rukou planovace
	sInterruptCtl.Set_Mask_IRQ(false);

	// vynutime prvni spusteni planovace
	sProcessMgr.Schedule();

	// tohle uz se mockrat nespusti - dalsi IRQ preplanuje procesor na nejaky z tasku (bud systemovy nebo uzivatelsky)
	while (true)
		;
	
	return 0;
}
