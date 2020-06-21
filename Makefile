OBJS = main_2.o ftp.o

test2 : $(OBJS)
%.o : %.c
	gcc -c -g $^ -o $@

.PHONY:clean
clean:
	-rm -rf $(OBJS)