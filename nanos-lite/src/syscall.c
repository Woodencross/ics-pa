#include "common.h"
#include "syscall.h"

int sys_none(){
  return 1;
}

void sys_exit(int a){
  _halt(a);
}

int sys_write(int fd,char* buf,int len){
  if(fd==1||fd==2){
    //char c;
    for(int i=0;i<len;i++){
      //memcpy(&c,buf+i,1);
      _putc(buf[i]);
    }
    return len;
  }
  else{
    panic("Unhandled fd =%d in sys_write",fd);
  }
  return -1;
}

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1]=SYSCALL_ARG2(r);
  a[2]=SYSCALL_ARG3(r);
  a[3]=SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_none:
    SYSCALL_ARG1(r)=sys_none();
    break;

    case SYS_exit:
    sys_exit(a[1]);
    break;

    case SYS_write:
    SYSCALL_ARG1(r)=sys_write((int)a[1],(char*)a[2],(int)a[3]);
    break;

    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
