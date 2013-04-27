CFLAGS += -DUSE_SELECT -Wall

OBJS_SERVER = test_server.o cfg_ipc_server.o cfg_fdevent.o cfg_fdevent_select.o

OBJS_CLIENT = cfg_ipc_client.o

TARGET_SERVER = cfg_ipc_server

TARGET_CLIENT = cfg_ipc_client

.PHONY:all
all: clean $(TARGET_SERVER) $(TARGET_CLIENT)
	
$(TARGET_SERVER): $(OBJS_SERVER)
	gcc $(OBJS_SERVER) -o $(@)

$(TARGET_CLIENT): $(OBJS_CLIENT)
	gcc $(OBJS_CLIENT) -o $(@)	

.PHONY:clean
clean: 
	rm -rf *.o $(TARGET_SERVER) $(TARGET_CLIENT)
	
