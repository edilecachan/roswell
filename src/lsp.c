#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "opt.h"
#include "util.h"
char** argv_orig;
int argc_orig;
struct opts* global_opt;
struct opts* local_opt=NULL;

extern int cmd_list(int argc,char **argv,struct sub_command* cmd);
extern int cmd_download(int argc,char **argv,struct sub_command* cmd);
extern int cmd_tar(int argc,char **argv,struct sub_command* cmd);
extern int cmd_version(int argc,char **argv,struct sub_command* cmd);
extern int cmd_config(int argc,char **argv,struct sub_command* cmd);
extern int cmd_help(int argc,char **argv,struct sub_command* cmd);

extern void register_cmd_run(void);
extern void register_cmd_install(void);
int verbose=0;
int rc=1;
//dummy
int cmd_notyet(int argc,char **argv,struct sub_command* cmd)
{
  printf("notyet\n");
  return 1;
}

LVal top_commands =(LVal)NULL;
LVal top_options =(LVal)NULL;

LVal top_helps =(LVal)NULL;
LVal subcommand_name=(LVal)NULL;

int proccmd(int argc,char** argv,LVal option,LVal command) {
  int i;
  int pos;
  if(verbose>0)
    fprintf(stderr,"proccmd:%s\n",argv[0]);
  if(argv[0][0]=='-' || argv[0][0]=='+') {
    if(argv[0][0]=='-' &&
       argv[0][1]=='-') { /*long option*/
      LVal p;
      for(p=option;p;p=Next(p)) {
        struct sub_command* fp=firstp(p);
        if(strcmp(&argv[0][2],fp->name)==0) {
          int result= fp->call(argc,argv,fp);
          if(fp->terminating) {
            if(verbose>0)
              fprintf(stderr,"terminating:%s\n",argv[0]);
            exit(result);
          }else {
            return result;
          }
        }
      }
      if(verbose>0)
        fprintf(stderr,"proccmd:invalid? %s\n",argv[0]);
    }else { /*short option*/
      if(argv[0][1]!='\0') {
        LVal p;
        for(p=option;p;p=Next(p)) {
          struct sub_command* fp=firstp(p);
          if(fp->short_name&&strcmp(argv[0],fp->short_name)==0) {
            int result= fp->call(argc,argv,fp);
            if(fp->terminating) {
              if(verbose>0)
                fprintf(stderr,"terminating:%s\n",argv[0]);
              exit(result);
            }else {
              return result;
            }
          }
        }
      }
      /* invalid */
    }
  }else if((pos=position_char("=",argv[0]))!=-1) {
    char *l,*r;
    l=subseq(argv[0],0,pos);
    r=subseq(argv[0],pos+1,0);
    if(r)
      set_opt(&local_opt,l,r,0);
    else{
      struct opts* opt=global_opt;
      struct opts** opts=&opt;
      unset_opt(opts, l);
      global_opt=*opts;
    }
    s(l),s(r);
    return proccmd(argc-1,&(argv[1]),option,command);
  }else {
    char* tmp[]={"help"};
    LVal p;
    for(p=command;p;p=Next(p)) {
      struct sub_command* fp=firstp(p);
      if(fp->name&&(strcmp(fp->name,argv[0])==0||strcmp(fp->name,"*")==0)) {
        subcommand_name=conss(q(fp->name),subcommand_name);
        exit(fp->call(argc,argv,fp));
      }
    }
    fprintf(stderr,"invalid command\n");
    proccmd(1,tmp,top_options,top_commands);
  }
  return 1;
}

int opt_top(int argc,char** argv,struct sub_command* cmd) {
  const char* arg=cmd->name;
  if(arg && argc>1)
    set_opt(&local_opt,arg,argv[1],0);
  return 2;
}

int opt_top_verbose(int argc,char** argv,struct sub_command* cmd) {
  if(strcmp(cmd->name,"verbose")==0) {
    ++verbose;
  }else if(strcmp(cmd->name,"quiet")==0) {
    --verbose;
  }
  if(verbose>0)
    fprintf(stderr,"opt_verbose:%s\n",cmd->name);
  return 1;
}

int opt_top_rc(int argc,char** argv,struct sub_command* cmd) {
  if(strcmp(cmd->name,"rc")==0) {
    rc=1;
  }else if(strcmp(cmd->name,"no-rc")==0) {
    rc=0;
  }
  if(verbose>0)
    fprintf(stderr,"opt_rc:%s\n",cmd->name);
  return 1;
}

int opt_top_build0(int argc,char** argv,struct sub_command* cmd) {
  if(cmd->name) {
    char* current=get_opt("program");
    current=cat(current?current:"","(:",cmd->name,")",NULL);
    set_opt(&local_opt,"program",current,0);
  }
  return 1;
}

int opt_top_build(int argc,char** argv,struct sub_command* cmd) {
  if(cmd->name && argc>1) {
    char* current=get_opt("program");
    char* escaped=escape_string(argv[1]);
    int len=strlen(escaped)+strlen(cmd->name)+7;
    current=cat(current?current:"","(:",cmd->name," \"",escaped,"\")",NULL);
    s(escaped);
    set_opt(&local_opt,"program",current,0);
  }
  return 2;
}

LVal register_runtime_options(LVal opt) {
  /*opt=add_command(opt,"file","-f",opt_top_build,1,0,"include lisp FILE while building","FILE");*/
  opt=add_command(opt,"load","-l",opt_top_build,1,0,"load lisp FILE while building","FILE");
  opt=add_command(opt,"source-registry","-S",opt_top_build,1,0,"override source registry of asdf systems","X");
  opt=add_command(opt,"system","-s",opt_top_build,1,0,"load asdf SYSTEM while building","SYSTEM");
  opt=add_command(opt,"load-system",NULL,opt_top_build,1,0,"same as above (buildapp compatibility)","SYSTEM");
  opt=add_command(opt,"package","-p",opt_top_build,1,0,"change current package to PACKAGE","PACKAGE");
  opt=add_command(opt,"system-package","-sp",opt_top_build,1,0,"combination of -s SP and -p SP","SP");
  opt=add_command(opt,"eval","-e",opt_top_build,1,0,"evaluate FORM while building","FORM");
  opt=add_command(opt,"require",NULL,opt_top_build,1,0,"require MODULE while building","MODULE");
  opt=add_command(opt,"quit","-q",opt_top_build0,1,0,"quit lisp here",NULL);

  opt=add_command(opt,"restart","-r",cmd_notyet,1,0,"restart from build by calling (FUNC)","FUNC");
  opt=add_command(opt,"entry","-E",cmd_notyet,1,0,"restart from build by calling (FUNC argv)","FUNC");
  opt=add_command(opt,"init","-i",cmd_notyet,1,0,"evaluate FORM after restart","FORM");
  opt=add_command(opt,"print","-ip",cmd_notyet,1,0,"evaluate and princ FORM after restart","FORM");
  opt=add_command(opt,"write","-iw",cmd_notyet,1,0,"evaluate and write FORM after restart","FORM");

  opt=add_command(opt,"final","-F",cmd_notyet,1,0,"evaluate FORM before dumping IMAGE","FORM");

  /* opt=add_command(opt,"include","-I",cmd_notyet,1,0,"runtime PATH to cl-launch installation","PATH"); */
  /* opt=add_command(opt,"no-include","+I",cmd_notyet,1,0,"disable cl-launch installation feature",NULL); */
  opt=add_command(opt,"rc","-R",cmd_notyet,1,0,"try read /etc/rosrc, ~/.rosrc",NULL);
  opt=add_command(opt,"no-rc","+R",opt_top_rc,1,0,"skip /etc/rosrc, ~/.rosrc",NULL);
  opt=add_command(opt,"quicklisp","-Q",cmd_notyet,1,0,"use quicklisp",NULL);
  opt=add_command(opt,"no-quicklisp","+Q",cmd_notyet,1,0,"do not use quicklisp",NULL);
  opt=add_command(opt,"verbose","-v",opt_top_verbose,1,0,"be quite noisy while building",NULL);
  opt=add_command(opt,"quiet",NULL,opt_top_verbose,1,0,"be quite quiet while building (default)",NULL);
  return opt;
}

int main (int argc,char **argv) {
  int i;
  argv_orig=argv;
  argc_orig=argc;
  char* _help;
  /*options*/
  /* toplevel */
  top_options=add_command(top_options,"wrap","-w",opt_top,1,0,"load lisp FILE while building","CODE");
  top_options=add_command(top_options,"image","-m",opt_top,1,0,"build from Lisp image IMAGE","IMAGE");
  top_options=add_command(top_options,"lisp","-L",opt_top,1,0,"try use these LISP implementation","NAME");
  top_options=register_runtime_options(top_options);

  /* abbrevs */
  top_options=add_command(top_options,"version","-V",cmd_version,0,1,NULL,NULL);
  top_options=add_command(top_options,"help","-h",cmd_help,0,1,NULL,NULL);
  top_options=add_command(top_options,"help","-?",cmd_help,0,1,NULL,NULL);

  top_options=nreverse(top_options);
  /*commands*/
  register_cmd_install();
  top_commands=add_command(top_commands,"config"  ,NULL,cmd_config,1,1,"Get and set options",NULL);

  /*         {"list",NULL,cmd_list,1,1}, */
  /*         {"set",NULL,cmd_notyet,0,1}, */
  top_commands=add_command(top_commands,"version" ,NULL,cmd_version,1,1,"Show the "PACKAGE" version information",NULL);
  top_commands=add_command(top_commands,"tar"     ,NULL,cmd_tar,0,1,NULL,NULL);
  top_commands=add_command(top_commands,"download",NULL,cmd_download,0,1,NULL,NULL);
  top_commands=add_command(top_commands,"help",NULL,cmd_help,0,1,NULL,NULL);
  register_cmd_run();
  top_commands=nreverse(top_commands);
  _help=cat("Usage: ",argv_orig[0]," [OPTIONS] [Command arguments...]  \n",
            "Usage: ",argv_orig[0]," [OPTIONS] [[--] script-path arguments...]  \n\n",NULL);
  top_helps=add_help(top_helps,NULL,_help,top_commands,top_options,NULL,NULL);
  s(_help);

  char* path=s_cat(homedir(),q("config"),NULL);
  global_opt=load_opts(path);
  struct opts** opts=&global_opt;
  unset_opt(opts,"program");
  s(path);
  if(argc==1) {
    char* tmp[]={"help"};
    proccmd(1,tmp,top_options,top_commands);
  }else
    for(i=1;i<argc;i+=proccmd(argc-i,&argv[i],top_options,top_commands));
  if(get_opt("program")) {
    char* tmp[]={"run","--"};
    proccmd(2,tmp,top_options,top_commands);
  }
  free_opts(global_opt);
}
