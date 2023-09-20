obj-m += driver.o 
KDIR?=/lib/modules/3.14.56-udooneo-02054-gc460cad/build

all:
	$(MAKE) -C $(KDIR) M=$$PWD
	
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
    
