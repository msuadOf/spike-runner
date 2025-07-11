#include <getopt.h>     // included for `getopt_long`
#include <libgen.h>     // included for `basename`
#include <stdlib.h>     // included for `EXIT_SUCCESS|EXIT_FAILURE`


#include "mmu.h"
#include "sim.h"
#include <iostream>


#define NR_GPR  32
#define CONFIG_MSIZE (2048 << 20)

static std::vector<std::pair<reg_t, abstract_device_t*>> difftest_plugin_devices;
static std::vector<std::string> difftest_htif_args;
static std::vector<std::pair<reg_t, mem_t*>> difftest_mem(
    1, std::make_pair(reg_t(DRAM_BASE), new mem_t(CONFIG_MSIZE)));
static debug_module_config_t difftest_dm_config = {
  .progbufsize = 2,
  .max_sba_data_width = 0,
  .require_authentication = false,
  .abstract_rti = 0,
  .support_hasel = true,
  .support_abstract_csr_access = true,
  .support_abstract_fpr_access = true,
  .support_haltgroups = true,
  .support_impebreak = true
};


static sim_t* s = NULL;
static processor_t *p = NULL;
static state_t *state = NULL;



void sim_t::diff_step(uint64_t n) {
  for(int i=0; i < n; i++){
    //diff_get_regs((void*)-1);

    step(1);

    if(state->mcause->read()==3)
    {
      std::cout << std::endl << "==hit ebreak==" << std::endl;
      break;
    }
    //printf("==%d\n",i);
  }
  // step(n);
}
void sim_t::diff_get_regs(void* diff_context) {
if(diff_context==NULL)
{  for (int i = 0; i < NR_GPR; i++) {
    std::cout << "r" << i <<":"<< std::hex << state->XPR[i]  << std::endl;
      }}
      std::cout << "pc"  <<":"<< std::hex << state->pc  << std::endl;
}
// void sim_t::diff_get_regs(void* diff_context) {
//   struct diff_context_t* ctx = (struct diff_context_t*)diff_context;
//   for (int i = 0; i < NR_GPR; i++) {
//     ctx->gpr[i] = state->XPR[i];
//   }
//   ctx->pc = state->pc;
  
//   ctx->csr.mtvec   = state->mtvec->read();  
//   ctx->csr.mstatus = state->mstatus->read();
//   ctx->csr.mepc    = state->mepc->read();    
//   ctx->csr.mcause  = state->mcause->read();  
// }

// void sim_t::diff_set_regs(void* diff_context) {
//   struct diff_context_t* ctx = (struct diff_context_t*)diff_context;
//   for (int i = 0; i < NR_GPR; i++) {
//     state->XPR.write(i, (sword_t)ctx->gpr[i]);
//   }
//   state->pc = ctx->pc;

//   state->mtvec->write(ctx->csr.mtvec);
//   state->mstatus->write(ctx->csr.mstatus);
//   state->mepc->write(ctx->csr.mepc);
//   state->mcause->write(ctx->csr.mcause);
// }

void sim_t::diff_memcpy(reg_t dest, void* src, size_t n) {
  mmu_t* mmu = p->get_mmu();
  printf("copy %ld bytes\n",n);
  for (size_t i = 0; i < n; i++) {
    mmu->store<uint8_t>(dest+i, *((uint8_t*)src+i));
  }
}
char* img_buf;
static long load_img(char* img_file) {
  if (img_file == NULL) {
    // Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  // Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  // Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  img_buf= (char*)malloc(size);
  int ret = fread(img_buf, size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}
void sim_t::diff_init(int port) {


      p = get_core("0");
      state = p->get_state();
      //===================
      state->pc = 0x80000000;

  return ;
}
static const struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  // TODO::
  {"img", required_argument, NULL, 'i'},
  {"next-generation", no_argument, NULL, 'n'},
  {"traditional", no_argument, NULL, 't'},
  // ----------
  {NULL, 0, NULL, 0}};

  int main(int argc, char *argv[])
  {
      int optc;
      const char *program_name = basename(argv[0]);
      int lose = 0;
  
      // TODO::
      int t = 0, n = 0;
      char *imgpath = NULL;
      // ----------
  
      while ((optc = getopt_long(argc, argv, "hVi:nt", longopts, NULL)) != -1)
          switch (optc) {
              /* One goal here is having --help and --version exit immediately,
                 per GNU coding standards.  */
              case 'h':
                  printf("%s\n",program_name);
                  exit(EXIT_SUCCESS);
                  break;
              case 'V':
                  printf("%s\n",program_name);
                  exit(EXIT_SUCCESS);
                  break;
              case 'i':
                  imgpath = optarg;
                  if (imgpath == NULL)
                  {
                      printf("Error: No image path provided\n");
                      assert(imgpath);;
                  }
                  break;

              default:
                  
                  break;
          }

          

  difftest_htif_args.push_back("");
  const char *isa = "RV"  "32"  "I" "MAFDC";
  cfg_t cfg(/*default_initrd_bounds=*/std::make_pair((reg_t)0, (reg_t)0),
            /*default_bootargs=*/nullptr,
            /*default_isa=*/isa,
            /*default_priv=*/DEFAULT_PRIV,
            /*default_varch=*/DEFAULT_VARCH,
            /*default_misaligned=*/false,
            /*default_endianness*/endianness_little,
            /*default_pmpregions=*/16,
            /*default_mem_layout=*/std::vector<mem_cfg_t>(),
            /*default_hartids=*/std::vector<size_t>{0,1},
            /*default_real_time_clint=*/false,
            /*default_trigger_count=*/4);
  s = new sim_t(&cfg, false,
      difftest_mem, difftest_plugin_devices, difftest_htif_args,
      difftest_dm_config, "procs_debug.trace", false, NULL,
      false,
      NULL,
      false);
  s->set_procs_debug(true);

  s->diff_init(0);
  int size=load_img(imgpath);
  s->diff_memcpy(0x80000000, img_buf, size);
  // s->diff_get_regs(NULL);
  // while(1){
    s->diff_step(-1);
    // s->diff_get_regs(NULL);
  // }
  std::cout << "hello" << std::endl;
}