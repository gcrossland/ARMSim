/* -------------------------------------------------------------------
   ProcessSim Library V1.06                             HEADER FILE
   (c) Geoffrey Crossland 2000, 2002

   V1.00 : Provides a library for simulating systems via a set of
           linked functional units.
   V1.05 : Provides Functional Units with a consistant way of
           producing user output. Implements global pipes,
           accessable by all Functional Units.
   V1.06 : The directory in which output files are placed is now
           configurable and the system is a little more forgiving
           when the directory doesn't exist or is full. Uses
           something slightly more efficient to write to the output
           file. The refresh rate of step files is now
           configurable.
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
#define PROCESSSIM_FU_NULL  ((void*) 0)


struct processsim_fu_table_element
{
  void (*executor)(void*);
  void *workspace;
  struct processsim_pipe_table_element **pipes;
  i32 output_x;
  i32 output_y;
  char *output_buffer;
  i32 output_buffer_size;
  struct processsim_fu_table_element *next;
};

struct processsim_pipe_table_element
{
  void *data;
  struct processsim_fu_table_element *fus[2];
  struct processsim_pipe_table_element *next;
};



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern struct processsim_fu_table_element *processsim_fu_table;
extern struct processsim_pipe_table_element *processsim_pipe_table;
extern struct processsim_pipe_table_element **processsim_gpipe_table;
extern i32 processsim_main_clocks_count;
extern i32 processsim_main_output_step;
extern char *processsim_main_output_path;
extern clock_t processsim_main_secs_start;



extern void processsim_init (i32 gpipe_count,char *output_path,
                             i32 output_step);
extern void processsim_lock (void);
extern i32 processsim_main (ui8 output);
extern i32 processsim_main_clocks (void);
extern f32 processsim_main_secs (void);
extern ui8 processsim_main_outputting (void);
extern void processsim_main_output (void);
extern void processsim_main_output_file (char *filename,ui8 dynamic,
                                         i32 cycle,char *output,
                                         i32 output_size);
extern void *processsim_fu_gen (void (*executor)(void*),
                                i32 pipe_count,i32 output_buffer_size,
                                i32 output_x,i32 output_y);
extern void *processsim_pipe_gen (void *fu0,i32 fu0_pipe,void *fu1,
                           i32 fu1_pipe);
extern void *processsim_pipe_read (void *fu,i32 pipe);
extern void processsim_pipe_write (void *fu,i32 pipe,void *value);
extern void *processsim_workspace_read (void *fu);
extern void processsim_workspace_write (void *fu,void *value);
extern char *processsim_output_read (void *fu);
