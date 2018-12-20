#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int flags;         // flags for open() indicating read or write
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    _exit(0);
  
  switch(cmd->type){
  default:
    fprintf(stderr, "unknown runcmd\n");
    _exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      _exit(0);
    //fprintf(stderr, "exec not implemented\n");
    // Your code here ...
       
    if (access(ecmd->argv[0], F_OK) == 0){      //判断argv->[0]能否被解释为绝对路径
      if(execv(ecmd->argv[0], ecmd->argv)==-1){
        fprintf(stderr, "exec failed\n");
      }
    }
    else{
      char* abspath[2]={(char *)malloc((strlen(ecmd->argv[0])+6)*sizeof(char)),(char *)malloc((strlen(ecmd->argv[0])+10)*sizeof(char))};
      strcpy(abspath[0],"/bin/");
      strcpy(abspath[1],"/usr/bin/");
      strcat(abspath[0],ecmd->argv[0]);
      strcat(abspath[1],ecmd->argv[0]);
      if (access(abspath[0], F_OK) == 0){
        if(execv(abspath[0], ecmd->argv)==-1){
        fprintf(stderr, "exec failed\n");
        }
      }
      else if (access(abspath[1], F_OK) == 0){
        if(execv(abspath[1], ecmd->argv)==-1){
        fprintf(stderr, "exec failed\n");
        }
      }
      else{
        fprintf(stderr, "can not find cmd%s\n",ecmd->argv[0]);
      }
    }
    
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    //fprintf(stderr, "redir not implemented\n");
    // Your code here ...

    close(rcmd->fd);                                // 关闭父进程打开的输入or输出文件
    if(open(rcmd->file,rcmd->flags,S_IRUSR|S_IWUSR)<0){   
    //if(open(rcmd->file,rcmd->flags,0644)>=0){   //打开子进程被要求重定向的文件，注意flag已被设置好，权限
      
      fprintf(stderr, "can not open file: %s\n",rcmd->file);
    }
    else{
      runcmd(rcmd->cmd);
    }

    
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    //fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    /*
    By convention, a process reads from file descriptor 0 (standard input), writes output to file
    descriptor 1 (standard output), and writes error messages to file descriptor 2 (standard error). 
    The shell ensures that it always has three file descriptors open (8707), 
    which are by default file descriptors for the console.
    A newly allocated file descriptor is always the lowest-numbered unused descriptor of the current process.
    */




    int piped[2];                               
    if(pipe(piped)==-1){                         //利用pipe函数构建管道，其中pipe[0]为读取端，pipe[1]为输入端，现在文件描述符状态为：
                                                 //0:stdin, 1:stdout , 2:stderr , 3:pipe[0] ,4:pipe[1]  
      fprintf(stderr, "can not open pipe\n");
    }
    if(fork()==0){                    //建立子进程，
      close(1);                      //对于|左边的命令需要关闭stdout，替换为pipe[1]
      dup(piped[1]);                  //dup产生当前空闲最小的文件描述符来绑定参数里的文件描述符
      close(piped[0]);
      close(piped[1]);
      runcmd(pcmd->left);            //0:stdin , 1:pipe[0] , 2:stderr
    }
    if(fork()==0){               //同上右边的命令需要将stdin改为pipe[0]
      close(0);
      dup(piped[0]);
      close(piped[0]);
      close(piped[1]);
      runcmd(pcmd->right);              //0:pipe[1] , 1:stdout, 2:stderr
    }   
    close(piped[0]);
    close(piped[1]);
    wait(NULL);
    wait(NULL);
    break;
  }    
  _exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");
  memset(buf, 0, nbuf);
  if(fgets(buf, nbuf, stdin) == 0)
    return -1; // EOF
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
