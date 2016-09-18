#
#ik_speed_test
#

.PHONY:ik_speed_test clean 

LFLAGS += -lpthread -lm

CPPFLAGS+=-g $(TARGET_CPPFLAGS) -I$(STAGING_DIR)/usr/include/glib-2.0 \
	-Wall -DCONFIG_NO_STDOUT_DEBUG -D_REENTRANT -D_GNU_SOURCE

OBJS+=ik_speed_test.o
TARGET	:= ik_speed_test

$(TARGET): $(OBJS)
	$(Q)$(CC) -o $@ $^ $(LDFLAGS) $(LFLAGS)

clean:
	rm -f ik_speed_test $(OBJS) 
