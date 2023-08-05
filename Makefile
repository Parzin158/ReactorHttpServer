CC = g++
TARGET = server
OBJDIR = obj
SRCDIR = src
INCDIR = inc

#获取源文件路径下的所有.cpp和.o文件名
SOURCE = $(wildcard ./src/*.cpp)
$(warning SOURCE is ${SOURCE})
OBJECT = $(patsubst %.cpp,./$(OBJDIR)/%.o, $(notdir $(filter %.cpp, $(SOURCE))))
$(warning OBJECT is ${OBJECT})

#头文件路径
INCLUDE := $(shell pwd)/$(INCDIR)
I_FLAGS := $(INCLUDE:%=-I% )
$(warning I_FLAGS is ${I_FLAGS})

#编译选项
CFLAGS := -g -O3 -w -lpthread -std=c++11
$(warning CFLAGS is ${CFLAGS})

$(OBJDIR)/%.o:$(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(I_FLAGS) ${CFLAGS} -c $< -o $@ 

$(TARGET):$(OBJECT)
	@$(CC) $^ -o $@

debug :
	@echo $(SOURCE)
	@echo $(OBJECT)
	@echo $(INCLUDE)

clean :
	@rm -f $(OBJECT) $(TARGET)

.PHOHY : $(TARGET) debug clean
