/* Inference for Llama 2 & Llama 3 / 3.1 Transformer model in pure C, int8 quantized forward pass. */

// L2E Addition
/* The Llama 2 Everywhere @trholding (Vulcan) fork   */

// ----------------------------------------------------------------------------
// L2E : Global Variables
// 

int buffertokens = 1;     // output token buffer size
int stats = 1;     // extended status info
int llamaver = 2; // llama version (default is 2, valid 2 & 3)
float rope_tf = 10000.0; // Rope tetha or frequency, 10000.0 => llama2, 500000.0 > llama3
int BOS = 1; // Beginning of Sentence token value, llama2 = 1 , llama3 = 128000
int EOS = 2; // End of Sentence token value, llama2 = 2 , llama3 = 128009 (end of text)
char system_template[1024]="";
char user_template[1024]="";

// ----------------------------------------------------------------------------
// L2E Humanoid : Linux Kernel Support Directives
// 

#define _DEFTOSTR(LSTR) #LSTR
#define DEFTOSTR(LSTR) _DEFTOSTR(LSTR)

#define LOOPSTATUS 0 // Status off

#ifndef LINUXK
#define OSPROMPT L2E$
#endif

#ifdef LINUXK
#define INC_BIN 
#define LLOOP
#define LOOPSTATUS 1 // Status on
#endif
   
// ----------------------------------------------------------------------------
// L2E Asteroid : Unikraft Unikernel Support Directives
// 

#ifdef UNIK
#define STRLIT 
#define LLOOP
#define LOOPSTATUS 1 // Status on
#endif

// ----------------------------------------------------------------------------
// INCBIN Embedding Support Directives
// https://github.com/graphitemaster/incbin

// String substitution macro needed to pass paths to INCBIN
#define ADDPATH(FPATH) TOSTR(FPATH)
#define TOSTR(FPATH) #FPATH

#ifdef INC_BIN // Support for embedding model and tokenizer

#define INCBIN_PREFIX emb_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

#ifndef MODPATH
#define MODPATH out/model.bin // default model path
#endif
#ifndef TOKPATH
#define TOKPATH tokenizer.bin // default tokenizer path
#endif

INCBIN(Model, ADDPATH(MODPATH)); // Model path is passed via makefile
INCBIN(Tokenizer, ADDPATH(TOKPATH)); // Tokenizer path is passed via makefile

#endif

// ----------------------------------------------------------------------------
// strliteral (STRLIT) Embedding Support Directives
// https://github.com/mortie/strliteral

#ifdef STRLIT
#include "model.h"
#include "tokenizer.h"
#endif

// ----------------------------------------------------------------------------
// Actually Portable Executable Format Preprocessor Directives

#ifdef COSMO_BLINK // Support ARM 64 Bit via Blink VM Emulation
__static_yoink("blink_linux_aarch64");  // for raspberry pi
__static_yoink("blink_xnu_aarch64");    // is apple silicon
#endif

#ifdef COSMO_METAL // Support VGA Console when running bare metal
__static_yoink("vga_console");
#endif

#ifdef COSMO_ZIP // Support embedded models via Zip Archive support
__static_yoink("zipos");
#endif

// ----------------------------------------------------------------------------
// BLAS Support

#if defined(CLBLAST) || defined(OPENBLAS) || defined(CBLAS) || defined(BLIS) || defined(MKL) || defined(ARMPL) || defined(AAF)
#define BLAS
#endif

#ifdef CLBLAST
#include <clblast_netlib_c.h>
#elif defined(BLIS)
#include "blis.h"
#include "cblas.h"
#elif defined(MKL)
#include "mkl.h"
#elif defined(ARMPL)
#include <armpl.h>
#elif defined(AAF)
#include <Accelerate/Accelerate.h>
#elif defined(OPENBLAS)
#include "cblas.h"
#elif defined(CBLAS)
#include <cblas.h>
#endif

// ----------------------------------------------------------------------------
// OpenMP and OpenACC Support

#ifdef OPENMP
#include <omp.h>
#endif

// Macro that makes a pragma enabled with string substitution
#define MKPRAGMA_(x) _Pragma (#x)
#define MK_PRAGMA(x) MKPRAGMA_(x)

// Portable OpenMP and OpenACC pragma macros
#ifdef OPENMP
#define ACCELS() MK_PRAGMA(omp parallel for)
#define ACCEL(...) MK_PRAGMA(omp parallel for private(__VA_ARGS__))
#define ACCELRD(VAR) MK_PRAGMA(omp parallel for reduction(+:VAR))
#elif defined(OPENACC)
#define ACCELS() MK_PRAGMA(acc parallel loop)
#define ACCEL(...) MK_PRAGMA(acc parallel loop private(__VA_ARGS__))
#define ACCELRD(VAR) MK_PRAGMA(acc parallel loop reduction(+:VAR))
#endif

// ----------------------------------------------------------------------------
// Standard Headers
// END L2E Addition

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#if defined _WIN32
    #include "win.h"
#else
    #include <unistd.h>
    #include <sys/mman.h>
#endif
// ----------------------------------------------------------------------------
// Globals
// L2E Addition
#if defined CAT
const int GS = 64; // group size 64 for Cheap Acceleration Tech :)
#else
int GS = 0; // group size global for quantization of the weights
#endif
// END L2E Addition

// ----------------------------------------------------------------------------
// Transformer model

typedef struct {
    int dim; // transformer dimension
    int hidden_dim; // for ffn layers
    int n_layers; // number of layers
    int n_heads; // number of query heads
    int n_kv_heads; // number of key/value heads (can be < query heads because of multiquery)
    int vocab_size; // vocabulary size, usually 256 (byte-level)
    int seq_len; // max sequence length
} Config;

typedef struct {
    int8_t* q;    // quantized values
    float* s; // scaling factors
} QuantizedTensor;

typedef struct {
    // token embedding table
    QuantizedTensor *q_tokens; // (vocab_size, dim)
    float* token_embedding_table; // same, but dequantized

    // weights for rmsnorms
    float* rms_att_weight; // (layer, dim) rmsnorm weights
    float* rms_ffn_weight; // (layer, dim)
    // weights for matmuls. note dim == n_heads * head_size
    QuantizedTensor *wq; // (layer, dim, n_heads * head_size)
    QuantizedTensor *wk; // (layer, dim, n_kv_heads * head_size)
    QuantizedTensor *wv; // (layer, dim, n_kv_heads * head_size)
    QuantizedTensor *wo; // (layer, n_heads * head_size, dim)
    // weights for ffn
    QuantizedTensor *w1; // (layer, hidden_dim, dim)
    QuantizedTensor *w2; // (layer, dim, hidden_dim)
    QuantizedTensor *w3; // (layer, hidden_dim, dim)
    // final rmsnorm
    float* rms_final_weight; // (dim,)
    // (optional) classifier weights for the logits, on the last layer
    QuantizedTensor *wcls;
} TransformerWeights;

typedef struct {
    // current wave of activations
    float *x; // activation at current time stamp (dim,)
    float *xb; // same, but inside a residual branch (dim,)
    float *xb2; // an additional buffer just for convenience (dim,)
    float *hb; // buffer for hidden dimension in the ffn (hidden_dim,)
    float *hb2; // buffer for hidden dimension in the ffn (hidden_dim,)
    QuantizedTensor xq; // quantized x (dim,)
    QuantizedTensor hq; // quantized hb (hidden_dim,)
    float *q; // query (dim,)
    float *k; // key (dim,)
    float *v; // value (dim,)
    float *att; // buffer for scores/attention values (n_heads, seq_len)
    float *logits; // output logits
    // kv cache
    float* key_cache;   // (layer, seq_len, dim)
    float* value_cache; // (layer, seq_len, dim)
} RunState;

typedef struct {
    Config config; // the hyperparameters of the architecture (the blueprint)
    TransformerWeights weights; // the weights of the model
    RunState state; // buffers for the "wave" of activations in the forward pass
    // some more state needed to properly clean up the memory mapping (sigh)
    int fd; // file descriptor for memory mapping
    float* data; // memory mapped data pointer
    ssize_t file_size; // size of the checkpoint file in bytes
} Transformer;

void malloc_run_state(RunState* s, Config* p) {
    // we calloc instead of malloc to keep valgrind happy
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    s->x = calloc(p->dim, sizeof(float));
    s->xb = calloc(p->dim, sizeof(float));
    s->xb2 = calloc(p->dim, sizeof(float));
    s->hb = calloc(p->hidden_dim, sizeof(float));
    s->hb2 = calloc(p->hidden_dim, sizeof(float));
    s->xq = (QuantizedTensor) { .q = calloc(p->dim, sizeof(int8_t)), .s = calloc(p->dim, sizeof(float)) };
    s->hq = (QuantizedTensor) { .q = calloc(p->hidden_dim, sizeof(int8_t)), .s = calloc(p->hidden_dim, sizeof(float)) };
    s->q = calloc(p->dim, sizeof(float));
    s->k = calloc(kv_dim, sizeof(float));
    s->v = calloc(kv_dim, sizeof(float));
    s->att = calloc(p->n_heads * p->seq_len, sizeof(float));
    s->logits = calloc(p->vocab_size, sizeof(float));
    s->key_cache = calloc(p->n_layers * p->seq_len * kv_dim, sizeof(float));
    s->value_cache = calloc(p->n_layers * p->seq_len * kv_dim, sizeof(float));
    // ensure all mallocs went fine
    if (!s->x || !s->xb || !s->xb2 || !s->hb || !s->hb2 || !s->q
     || !s->k || !s->v || !s->att || !s->logits || !s->key_cache
     || !s->value_cache) {
        fprintf(stderr, "malloc failed!\n");
        exit(EXIT_FAILURE);
    }
}

void free_run_state(RunState* s) {
    free(s->x);
    free(s->xb);
    free(s->xb2);
    free(s->hb);
    free(s->hb2);
    free(s->xq.q);
    free(s->xq.s);
    free(s->hq.q);
    free(s->hq.s);
    free(s->q);
    free(s->k);
    free(s->v);
    free(s->att);
    free(s->logits);
    free(s->key_cache);
    free(s->value_cache);
}

// ----------------------------------------------------------------------------
// Quantization functions

void dequantize(QuantizedTensor *qx, float* x, int n) {
// L2E Addition
    #ifdef ACCEL
    ACCELS() // OMP/OACC Macro
    #endif
// END L2E Addition
    for (int i = 0; i < n; i++) {
        x[i] = qx->q[i] * qx->s[i / GS];
    }
}

void quantize(QuantizedTensor *qx, float* x, int n) {
    int num_groups = n / GS;
    float Q_MAX = 127.0f;

// L2E Addition
    #ifdef ACCEL
    ACCELS() // OMP/OACC Macro
    #endif
// END L2E Addition
    for (int group = 0; group < num_groups; group++) {

        // find the max absolute value in the current group
        float wmax = 0.0;
        for (int i = 0; i < GS; i++) {
            float val = fabs(x[group * GS + i]);
            if (val > wmax) {
                wmax = val;
            }
        }

        // calculate and write the scaling factor
        float scale = wmax / Q_MAX;
        qx->s[group] = scale;

        // calculate and write the quantized values
        for (int i = 0; i < GS; i++) {
            float quant_value = x[group * GS + i] / scale; // scale
            int8_t quantized = (int8_t) round(quant_value); // round and clamp
            qx->q[group * GS + i] = quantized;
        }
    }
}

/* initialize `n` x quantized tensor (with `size_each` elements), starting from memory pointed at *ptr */
QuantizedTensor *init_quantized_tensors(void **ptr, int n, int size_each) {
    void *p = *ptr;
    QuantizedTensor *res = malloc(n * sizeof(QuantizedTensor));
    for(int i=0; i<n; i++) {
        /* map quantized int8 values*/
        res[i].q = (int8_t*)p;
        p = (int8_t*)p + size_each;
        /* map scale factors */
        res[i].s = (float*)p;
        p = (float*)p + size_each / GS;
    }
    *ptr = p; // advance ptr to current position
    return res;
}

void memory_map_weights(TransformerWeights *w, Config* p, void* ptr, uint8_t shared_classifier) {
    int head_size = p->dim / p->n_heads;
    // first are the parameters that are kept in fp32 (the rmsnorm (1D) weights)
    float* fptr = (float*) ptr; // cast our pointer to float*
    w->rms_att_weight = fptr;
    fptr += p->n_layers * p->dim;
    w->rms_ffn_weight = fptr;
    fptr += p->n_layers * p->dim;
    w->rms_final_weight = fptr;
    fptr += p->dim;

    // now read all the quantized weights
    ptr = (void*)fptr; // now cast the pointer back to void*
    w->q_tokens = init_quantized_tensors(&ptr, 1, p->vocab_size * p->dim);
    // dequantize token embedding table
    w->token_embedding_table = malloc(p->vocab_size * p->dim * sizeof(float));
    dequantize(w->q_tokens, w->token_embedding_table, p->vocab_size * p->dim);

    w->wq = init_quantized_tensors(&ptr, p->n_layers, p->dim * (p->n_heads * head_size));
    w->wk = init_quantized_tensors(&ptr, p->n_layers, p->dim * (p->n_kv_heads * head_size));
    w->wv = init_quantized_tensors(&ptr, p->n_layers, p->dim * (p->n_kv_heads * head_size));
    w->wo = init_quantized_tensors(&ptr, p->n_layers, (p->n_heads * head_size) * p->dim);

    w->w1 = init_quantized_tensors(&ptr, p->n_layers, p->dim * p->hidden_dim);
    w->w2 = init_quantized_tensors(&ptr, p->n_layers, p->hidden_dim * p->dim);
    w->w3 = init_quantized_tensors(&ptr, p->n_layers, p->dim * p->hidden_dim);

    w->wcls = shared_classifier ? w->q_tokens : init_quantized_tensors(&ptr, 1, p->dim * p->vocab_size);
}

// L2E Addition
#if defined (INC_BIN) || defined(STRLIT)
void read_checkpoint(char* checkpoint, Config* config, TransformerWeights* weights, 
                     int* fd, float** data, ssize_t* file_size) {
    // Calculate the file size from the raw data
    *file_size = strlen(checkpoint);

    // memory map the Transformer weights into the data pointer
    *fd = -1; // No file descriptor is needed since we're not opening a file
    *data = (float*) checkpoint;

    // Create a byte pointer to navigate the data
    uint8_t* ptr = (uint8_t*) *data;

    // read in magic number (uint32), has to be 0x616b3432, i.e. "ak42" in ASCII
    uint32_t magic_number = *(uint32_t*) ptr;
    ptr += sizeof(uint32_t);
    if (magic_number != 0x616b3432) { fprintf(stderr, "Bad magic number\n"); exit(EXIT_FAILURE); }

    // read in the version number (uint32), has to be 2
    int version = *(int*) ptr;
    ptr += sizeof(int);
    if (version != 2) { fprintf(stderr, "Bad version %d, need version 2\n", version); exit(EXIT_FAILURE); }

    int header_size = 256; // the header size for version 2 in bytes

    // read in the Config
    memcpy(config, ptr, sizeof(Config));
    ptr += sizeof(Config);

    // read in flags
    uint8_t shared_classifier = *(uint8_t*) ptr;
    ptr += sizeof(uint8_t);

    int group_size = *(int*) ptr;
    ptr += sizeof(int);

// L2E Addition
    #ifndef CAT
    GS = group_size; // set as global, as it will be used in many places
    #endif
// END L2E Addition

    void* weights_ptr = ((char*)*data) + header_size; // skip header bytes
    memory_map_weights(weights, config, weights_ptr, shared_classifier);
}
#else
// END L2E Addition

void read_checkpoint(char* checkpoint, Config* config, TransformerWeights* weights,
                     int* fd, float** data, ssize_t* file_size) {
    FILE *file = fopen(checkpoint, "rb");
    if (!file) { fprintf(stderr, "Couldn't open file %s\n", checkpoint); exit(EXIT_FAILURE); }
    // read in magic number (uint32), has to be 0x616b3432, i.e. "ak42" in ASCII
    uint32_t magic_number;
    if (fread(&magic_number, sizeof(uint32_t), 1, file) != 1) { exit(EXIT_FAILURE); }
    if (magic_number != 0x616b3432) { fprintf(stderr, "Bad magic number\n"); exit(EXIT_FAILURE); }
    // read in the version number (uint32), has to be 2
    int version;
    if (fread(&version, sizeof(int), 1, file) != 1) { exit(EXIT_FAILURE); }
    if (version != 2) { fprintf(stderr, "Bad version %d, need version 2\n", version); exit(EXIT_FAILURE); }
    int header_size = 256; // the header size for version 2 in bytes
    // read in the Config
    if (fread(config, sizeof(Config), 1, file) != 1) { exit(EXIT_FAILURE); }
    // read in flags
    uint8_t shared_classifier; // a byte to indicate if the classifier is shared
    if (fread(&shared_classifier, sizeof(uint8_t), 1, file) != 1) { exit(EXIT_FAILURE); }
    int group_size; // the group size used in quantization
    if (fread(&group_size, sizeof(int), 1, file) != 1) { exit(EXIT_FAILURE); }

// L2E Addition
    #ifndef CAT
    GS = group_size; // set as global, as it will be used in many places
    #endif
// END L2E Addition

    // figure out the file size
    fseek(file, 0, SEEK_END); // move file pointer to end of file
    *file_size = ftell(file); // get the file size, in bytes
    fclose(file);
    // memory map the Transformer weights into the data pointer
    *fd = open(checkpoint, O_RDONLY); // open in read only mode
    if (*fd == -1) { fprintf(stderr, "open failed!\n"); exit(EXIT_FAILURE); }
    *data = mmap(NULL, *file_size, PROT_READ, MAP_PRIVATE, *fd, 0);
    if (*data == MAP_FAILED) { fprintf(stderr, "mmap failed!\n"); exit(EXIT_FAILURE); }
    void* weights_ptr = ((char*)*data) + header_size; // skip header bytes. char is 1 byte
    memory_map_weights(weights, config, weights_ptr, shared_classifier);
}
// L2E Addition
#endif
// END L2E Addition

void build_transformer(Transformer *t, char* checkpoint_path) {
    // read in the Config and the Weights from the checkpoint
    read_checkpoint(checkpoint_path, &t->config, &t->weights, &t->fd, &t->data, &t->file_size);
    // allocate the RunState buffers
    malloc_run_state(&t->state, &t->config);
}

void free_transformer(Transformer* t) {
    // free QuantizedTensors
    free(t->weights.q_tokens);
    free(t->weights.token_embedding_table);
    free(t->weights.wq);
    free(t->weights.wk);
    free(t->weights.wv);
    free(t->weights.wo);
    free(t->weights.w1);
    free(t->weights.w2);
    free(t->weights.w3);
    if(t->weights.wcls != t->weights.q_tokens) { free(t->weights.wcls); }
    // close the memory mapping
    if (t->data != MAP_FAILED) { munmap(t->data, t->file_size); }
    if (t->fd != -1) { close(t->fd); }
    // free the RunState buffers
    free_run_state(&t->state);
}

// ----------------------------------------------------------------------------
// neural net blocks; the dynamics of the Transformer

void rmsnorm(float* o, float* x, float* weight, int size) {
    // calculate sum of squares
    float ss = 0.0f;
// L2E Addition
    #ifdef BLAS
    ss = cblas_sdot(size, x, 1.0f, x, 1.0f);
    #else
    #ifdef ACCEL
    ACCELRD(ss) // OMP/OACC Macro
    #endif
// END L2E Addition
    for (int j = 0; j < size; j++) {
        ss += x[j] * x[j];
    }
// L2E Addition
    #endif
// END L2E Addition
    ss /= size;
    ss += 1e-5f;
    ss = 1.0f / sqrtf(ss);
    // normalize and scale
// L2E Addition
    #ifdef ACCEL
    ACCELS() // OMP/OACC Macro
    #endif
// END L2E Addition
    for (int j = 0; j < size; j++) {
        o[j] = weight[j] * (ss * x[j]);
    }
}

void softmax(float* x, int size) {
    // find max value (for numerical stability)
    float max_val = x[0];
    for (int i = 1; i < size; i++) {
        if (x[i] > max_val) {
            max_val = x[i];
        }
    }
    // exp and sum
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    // normalize
    for (int i = 0; i < size; i++) {
        x[i] /= sum;
    }
}

// L2E Addition
#ifdef CAT

void matmul(float* xout, QuantizedTensor *x, QuantizedTensor *w, int n, int d) {
    // W (d,n) @ x (n,) -> xout (d,)
    // by far the most amount of time is spent inside this little function
    // inputs to this function are both quantized

    int i;
    #ifdef ACCEL
    ACCEL(i) // OMP/OACC Macro
    #endif
    for (i = 0; i < d; i++) {

        float val = 0.0f;
        int32_t ival = 0;
        int in = i * n;

        // do the matmul in groups of GS
        int j;
        for (j = 0; j <= n - GS; j += GS) {
            // unroll the inner loop by a factor of 4
            for (int k = 0; k < GS; k += 4) {
                ival += ((int32_t) x->q[j + k]) * ((int32_t) w->q[in + j + k]);
                ival += ((int32_t) x->q[j + k + 1]) * ((int32_t) w->q[in + j + k + 1]);
                ival += ((int32_t) x->q[j + k + 2]) * ((int32_t) w->q[in + j + k + 2]);
                ival += ((int32_t) x->q[j + k + 3]) * ((int32_t) w->q[in + j + k + 3]);
            }
            val += ((float) ival) * w->s[(in + j) / GS] * x->s[j / GS];
            ival = 0;
        }

        xout[i] = val;
    }
}

#else
// END L2E Addition
void matmul(float* xout, QuantizedTensor *x, QuantizedTensor *w, int n, int d) {
    // W (d,n) @ x (n,) -> xout (d,)
    // by far the most amount of time is spent inside this little function
    // inputs to this function are both quantized

    int i;
// L2E Addition
    #ifdef ACCEL
    ACCEL(i) // OMP/OACC Macro
    #endif
// END L2E Addition
    for (i = 0; i < d; i++) {

        float val = 0.0f;
        int32_t ival = 0;
        int in = i * n;

        // do the matmul in groups of GS
        int j;
        for (j = 0; j <= n - GS; j += GS) {
            for (int k = 0; k < GS; k++) {
                ival += ((int32_t) x->q[j + k]) * ((int32_t) w->q[in + j + k]);
            }
            val += ((float) ival) * w->s[(in + j) / GS] * x->s[j / GS];
            ival = 0;
        }

        xout[i] = val;
    }
}
// L2E Addition
#endif 
// END L2E Addition 

float* forward(Transformer* transformer, int token, int pos) {

    // a few convenience variables
    Config* p = &transformer->config;
    TransformerWeights* w = &transformer->weights;
    RunState* s = &transformer->state;
    float *x = s->x;
    int dim = p->dim;
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    int kv_mul = p->n_heads / p->n_kv_heads; // integer multiplier of the kv sharing in multiquery
    int hidden_dim =  p->hidden_dim;
    int head_size = dim / p->n_heads;

    // copy the token embedding into x
    memcpy(x, w->token_embedding_table + token*dim, dim * sizeof(float));

    // forward all the layers
    for(int l = 0; l < p->n_layers; l++) {

        // attention rmsnorm
        rmsnorm(s->xb, x, w->rms_att_weight + l*dim, dim);

        // qkv matmuls for this position
        quantize(&s->xq, s->xb, dim);
        matmul(s->q, &s->xq, w->wq + l, dim, dim);
        matmul(s->k, &s->xq, w->wk + l, dim, kv_dim);
        matmul(s->v, &s->xq, w->wv + l, dim, kv_dim);

    // Llama 3.1 RoPE Scaling
        const float scale_factor = 8;
        const float low_freq_factor = 1;
        const float high_freq_factor = 4;
        const float old_context_len = 8192;

        const float low_freq_wavelen  = old_context_len / low_freq_factor;
        const float high_freq_wavelen = old_context_len / high_freq_factor;
    // END Llama 3.1 RoPE Scaling

        // RoPE relative positional encoding: complex-valued rotate q and k in each head
        for (int i = 0; i < dim; i+=2) {
            int head_dim = i % head_size;
// L2E Addition
            float freq = 1.0f / powf(rope_tf, head_dim / (float)head_size);

// Llama 3.1
            // Check if M_PI is defined? Sorry haven't coded in C in ages lol
            float wavelen = 2.0f * M_PI / freq;
            if      (wavelen < high_freq_wavelen) { freq = freq; }
            else if (wavelen > low_freq_wavelen)  { freq = freq / scale_factor; }
            else {
                float smooth = (old_context_len / wavelen - low_freq_factor) / \
                    (high_freq_factor - low_freq_factor);

                freq = (1.0 - smooth) * freq / scale_factor + smooth * freq;
            }
// END Llama 3.1

// END L2E Addition
            float val = pos * freq;
            float fcr = cosf(val);
            float fci = sinf(val);
            int rotn = i < kv_dim ? 2 : 1; // how many vectors? 2 = q & k, 1 = q only
            for (int v = 0; v < rotn; v++) {
                float* vec = v == 0 ? s->q : s->k; // the vector to rotate (query or key)
                float v0 = vec[i];
                float v1 = vec[i+1];
                vec[i]   = v0 * fcr - v1 * fci;
                vec[i+1] = v0 * fci + v1 * fcr;
            }
        }

        // save key,value at this time step (pos) to our kv cache
        int loff = l * p->seq_len * kv_dim; // kv cache layer offset for convenience
        float* key_cache_row = s->key_cache + loff + pos * kv_dim;
        float* value_cache_row = s->value_cache + loff + pos * kv_dim;
        memcpy(key_cache_row, s->k, kv_dim * sizeof(*key_cache_row));
        memcpy(value_cache_row, s->v, kv_dim * sizeof(*value_cache_row));

        // multihead attention. iterate over all heads
        int h;
// L2E Addition
        #ifdef ACCEL
        ACCEL(h) // OMP/OACC Macro
        #endif
// END L2E Addition
        for (h = 0; h < p->n_heads; h++) {
            // get the query vector for this head
            float* q = s->q + h * head_size;
            // attention scores for this head
            float* att = s->att + h * p->seq_len;
            // iterate over all timesteps, including the current one
            for (int t = 0; t <= pos; t++) {
                // get the key vector for this head and at this timestep
                float* k = s->key_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                // calculate the attention score as the dot product of q and k
                float score = 0.0f;
                for (int i = 0; i < head_size; i++) {
                    score += q[i] * k[i];
                }
                score /= sqrtf(head_size);
                // save the score to the attention buffer
                att[t] = score;
            }

            // softmax the scores to get attention weights, from 0..pos inclusively
            softmax(att, pos + 1);

            // weighted sum of the values, store back into xb
            float* xb = s->xb + h * head_size;
            memset(xb, 0, head_size * sizeof(float));
            for (int t = 0; t <= pos; t++) {
                // get the value vector for this head and at this timestep
                float* v = s->value_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                // get the attention weight for this timestep
                float a = att[t];
                // accumulate the weighted value into xb
                for (int i = 0; i < head_size; i++) {
                    xb[i] += a * v[i];
                }
            }
        }

        // final matmul to get the output of the attention
        quantize(&s->xq, s->xb, dim);
        matmul(s->xb2, &s->xq, w->wo + l, dim, dim);

        // residual connection back into x
// L2E Addition
        #ifdef ACCEL
        ACCELS() // OMP/OACC Macro
        #endif
// END L2E Addition
        for (int i = 0; i < dim; i++) {
            x[i] += s->xb2[i];
        }

        // ffn rmsnorm
        rmsnorm(s->xb, x, w->rms_ffn_weight + l*dim, dim);

        // Now for FFN in PyTorch we have: self.w2(F.silu(self.w1(x)) * self.w3(x))
        // first calculate self.w1(x) and self.w3(x)
        quantize(&s->xq, s->xb, dim);
        matmul(s->hb, &s->xq, w->w1 + l, dim, hidden_dim);
        matmul(s->hb2, &s->xq, w->w3 + l, dim, hidden_dim);

        // SwiGLU non-linearity
// L2E Addition
        #ifdef ACCEL
        ACCELS() // OMP/OACC Macro
        #endif
// END L2E Addition
        for (int i = 0; i < hidden_dim; i++) {
            float val = s->hb[i];
            // silu(x)=x*σ(x), where σ(x) is the logistic sigmoid
            val *= (1.0f / (1.0f + expf(-val)));
            // elementwise multiply with w3(x)
            val *= s->hb2[i];
            s->hb[i] = val;
        }

        // final matmul to get the output of the ffn
        quantize(&s->hq, s->hb, hidden_dim);
        matmul(s->xb, &s->hq, w->w2 + l, hidden_dim, dim);

        // residual connection
        for (int i = 0; i < dim; i++) {
            x[i] += s->xb[i];
        }
    }

    // final rmsnorm
    rmsnorm(x, x, w->rms_final_weight, dim);

    // classifier into logits
    quantize(&s->xq, x, dim);
    matmul(s->logits, &s->xq, w->wcls, dim, p->vocab_size);
    return s->logits;
}

// ----------------------------------------------------------------------------
// The Byte Pair Encoding (BPE) Tokenizer that translates strings <-> tokens

typedef struct {
    char *str;
    int id;
} TokenIndex;

typedef struct {
    char** vocab;
    float* vocab_scores;
    TokenIndex *sorted_vocab;
    int vocab_size;
    unsigned int max_token_length;
    unsigned char byte_pieces[512]; // stores all single-byte strings
} Tokenizer;

int compare_tokens(const void *a, const void *b) {
    return strcmp(((TokenIndex*)a)->str, ((TokenIndex*)b)->str);
}

void build_tokenizer(Tokenizer* t, char* tokenizer_path, int vocab_size) {
    // i should have written the vocab_size into the tokenizer file... sigh
    t->vocab_size = vocab_size;
    // malloc space to hold the scores and the strings
    t->vocab = (char**)malloc(vocab_size * sizeof(char*));
    t->vocab_scores = (float*)malloc(vocab_size * sizeof(float));
    t->sorted_vocab = NULL; // initialized lazily
    for (int i = 0; i < 256; i++) {
        t->byte_pieces[i * 2] = (unsigned char)i;
        t->byte_pieces[i * 2 + 1] = '\0';
    }
// L2E Addition
#if defined (INC_BIN) || defined(STRLIT)
    // Parse the data from tokenizer_path
    char* token_data = tokenizer_path;
    int token_data_offset = 0;

    // Read the max_token_length from token_data
    memcpy(&t->max_token_length, token_data, sizeof(int));
    token_data_offset += sizeof(int);

    int len;
    for (int i = 0; i < vocab_size; i++) {
        // Read the vocab_scores from token_data
        memcpy(t->vocab_scores + i, token_data + token_data_offset, sizeof(float));
        token_data_offset += sizeof(float);

        // Read the length of the vocabulary token
        memcpy(&len, token_data + token_data_offset, sizeof(int));
        token_data_offset += sizeof(int);

        // Allocate memory for the vocabulary token and copy the data
        t->vocab[i] = (char*)malloc(len + 1);
        memcpy(t->vocab[i], token_data + token_data_offset, len);
        t->vocab[i][len] = '\0'; // add the string terminating token
        token_data_offset += len;
    }
#else
// END L2E Addition
    // read in the file
    FILE *file = fopen(tokenizer_path, "rb");
    if (!file) { fprintf(stderr, "couldn't load %s\n", tokenizer_path); exit(EXIT_FAILURE); }
    if (fread(&t->max_token_length, sizeof(int), 1, file) != 1) { fprintf(stderr, "failed read\n"); exit(EXIT_FAILURE); }
    int len;
    for (int i = 0; i < vocab_size; i++) {
        if (fread(t->vocab_scores + i, sizeof(float), 1, file) != 1) { fprintf(stderr, "failed read\n"); exit(EXIT_FAILURE);}
        if (fread(&len, sizeof(int), 1, file) != 1) { fprintf(stderr, "failed read\n"); exit(EXIT_FAILURE); }
        t->vocab[i] = (char *)malloc(len + 1);
        if (fread(t->vocab[i], len, 1, file) != 1) { fprintf(stderr, "failed read\n"); exit(EXIT_FAILURE); }
        t->vocab[i][len] = '\0'; // add the string terminating token
    }
    fclose(file);
// L2E Addition
#endif
// END L2E Addition
}

void free_tokenizer(Tokenizer* t) {
    for (int i = 0; i < t->vocab_size; i++) { free(t->vocab[i]); }
    free(t->vocab);
    free(t->vocab_scores);
    free(t->sorted_vocab);
}

char* decode(Tokenizer* t, int prev_token, int token) {
    char *piece = t->vocab[token];
// L2E Addition
    // following BOS (1) or (2) token, sentencepiece decoder strips any leading whitespace (see PR #89)
    if (prev_token == BOS && piece[0] == ' ') { piece++; }
// END L2E Addition
    // careful, some tokens designate raw bytes, and look like e.g. '<0x01>'
    // parse this and convert and return the actual byte
    unsigned char byte_val;
    if (sscanf(piece, "<0x%02hhX>", &byte_val) == 1) {
        piece = (char*)t->byte_pieces + byte_val * 2;
    }
    return piece;
}

void safe_printf(char *piece) {
    // piece might be a raw byte token, and we only want to print printable chars or whitespace
    // because some of the other bytes can be various control codes, backspace, etc.
    if (piece == NULL) { return; }
    if (piece[0] == '\0') { return; }
    if (piece[1] == '\0') {
        unsigned char byte_val = piece[0];
        if (!(isprint(byte_val) || isspace(byte_val))) {
            return; // bad byte, don't print it
        }
    }
    printf("%s", piece);
}

int str_lookup(char *str, TokenIndex *sorted_vocab, int vocab_size) {
    // efficiently find the perfect match for str in vocab, return its index or -1 if not found
    TokenIndex tok = { .str = str }; // acts as the key to search for
    TokenIndex *res = bsearch(&tok, sorted_vocab, vocab_size, sizeof(TokenIndex), compare_tokens);
    return res != NULL ? res->id : -1;
}

void encode(Tokenizer* t, char *text, int8_t bos, int8_t eos, int *tokens, int *n_tokens) {
    // encode the string text (input) into an upper-bound preallocated tokens[] array
    // bos != 0 means prepend the BOS token, eos != 0 means append the EOS token
    if (text == NULL) { fprintf(stderr, "cannot encode NULL text\n"); exit(EXIT_FAILURE); }

    if (t->sorted_vocab == NULL) {
        // lazily malloc and sort the vocabulary
        t->sorted_vocab = malloc(t->vocab_size * sizeof(TokenIndex));
        for (int i = 0; i < t->vocab_size; i++) {
            t->sorted_vocab[i].str = t->vocab[i];
            t->sorted_vocab[i].id = i;
        }
        qsort(t->sorted_vocab, t->vocab_size, sizeof(TokenIndex), compare_tokens);
    }

    // create a temporary buffer that will store merge candidates of always two consecutive tokens
    // *2 for concat, +1 for null terminator +2 for UTF8 (in case max_token_length is 1)
    char* str_buffer = malloc((t->max_token_length*2 +1 +2) * sizeof(char));
    size_t str_len = 0;

    // start at 0 tokens
    *n_tokens = 0;

// L2E Addition
    // add optional BOS token, if desired
    if (bos) tokens[(*n_tokens)++] = BOS;
// END L2E Addition


    // add_dummy_prefix is true by default
    // so prepend a dummy prefix token to the input string, but only if text != ""
    // TODO: pretty sure this isn't correct in the general case but I don't have the
    // energy to read more of the sentencepiece code to figure out what it's doing

// L2E Addition
    if (llamaver == 2) {
        if (text[0] != '\0') {
            int dummy_prefix = str_lookup(" ", t->sorted_vocab, t->vocab_size);
            tokens[(*n_tokens)++] = dummy_prefix;
        }
    }
// END L2E Addition

    // Okay UTF-8 time. This will get messy. Here is the reference from Wikipedia:
    // Code point ↔ UTF-8 conversion
    // First code point	Last code point	Byte 1	Byte 2	Byte 3	Byte 4
    // U+0000	U+007F	    0xxxxxxx
    // U+0080	U+07FF	    110xxxxx	10xxxxxx
    // U+0800	U+FFFF	    1110xxxx	10xxxxxx	10xxxxxx
    // U+10000	U+10FFFF    11110xxx	10xxxxxx	10xxxxxx	10xxxxxx

    // process the raw (UTF-8) byte sequence of the input string
    for (char *c = text; *c != '\0'; c++) {

        // reset buffer if the current byte is ASCII or a leading byte
        // 0xC0 is 11000000, so (*c & 0xC0) keeps the first 2 bits and zeros the rest
        // 0x80 is 10000000
        // in UTF-8, all continuation bytes start with "10" in first two bits
        // so in English this is: "if this byte is not a continuation byte"
        if ((*c & 0xC0) != 0x80) {
            // this byte must be either a leading byte (11...) or an ASCII char (0x...)
            // => reset our location, as we're starting a new UTF-8 codepoint
            str_len = 0;
        }

        // append the current byte to the buffer
        str_buffer[str_len++] = *c; // ++ is post-increment, incremented after this line
        str_buffer[str_len] = '\0';

        // while the next character is a continuation byte, continue appending
        // but if there are too many of them, just stop to avoid overruning str_buffer size.
        if ((*(c+1) & 0xC0) == 0x80 && str_len < 4) {
            continue;
        }

        // ok c+1 is not a continuation byte, so we've read in a full codepoint
        int id = str_lookup(str_buffer, t->sorted_vocab, t->vocab_size);

        if (id != -1) {
            // we found this codepoint in vocab, add it as a token
            tokens[(*n_tokens)++] = id;
        } else {
            // byte_fallback encoding: just encode each byte as a token
            // +3 is here because the first 3 vocab elements are <unk>, <s>, </s>
            // so the individual bytes only start at index 3
            for (int i=0; i < str_len; i++) {
                tokens[(*n_tokens)++] = (unsigned char)str_buffer[i] + 3;
            }
        }
        str_len = 0; // protect against a sequence of stray UTF8 continuation bytes
    }

// L2E Addition
// merge the best consecutive pair or triple each iteration, according to the scores in vocab_scores
    while (1) {
        float best_score = -1e10;
        int best_id = -1;
        int best_idx = -1;
        int best_merge = 0; // length of the best merge sequence (2 for pair, 3 for triple)

        // try to find the best pair or triple to merge
        for (int i = 0; i < (*n_tokens - 1); i++) {
            // check if we can merge the pair (tokens[i], tokens[i+1])
            sprintf(str_buffer, "%s%s", t->vocab[tokens[i]], t->vocab[tokens[i+1]]);
            int id = str_lookup(str_buffer, t->sorted_vocab, t->vocab_size);
            if (id != -1 && t->vocab_scores[id] > best_score) {
                // this merge pair exists in vocab! record its score and position
                best_score = t->vocab_scores[id];
                best_id = id;
                best_idx = i;
                best_merge = 2;
            }

            // check if we can merge the triple (tokens[i], tokens[i+1], tokens[i+2])
            if (i < (*n_tokens - 2)) {
                sprintf(str_buffer, "%s%s%s", t->vocab[tokens[i]], t->vocab[tokens[i+1]], t->vocab[tokens[i+2]]);
                id = str_lookup(str_buffer, t->sorted_vocab, t->vocab_size);
                if (id != -1 && t->vocab_scores[id] > best_score) {
                    // this merge triple exists in vocab! record its score and position
                    best_score = t->vocab_scores[id];
                    best_id = id;
                    best_idx = i;
                    best_merge = 3;
                }
            }
        }

        if (best_idx == -1) {
            break; // we couldn't find any more pairs or triples to merge, so we're done
        }

        // merge the consecutive pair or triple (best_idx, best_idx+1[, best_idx+2]) into new token best_id
        tokens[best_idx] = best_id;
        // delete token(s) at position best_idx+1 (and optionally best_idx+2), shift the entire sequence back
        for (int i = best_idx + 1; i < (*n_tokens - best_merge + 1); i++) {
            tokens[i] = tokens[i + best_merge - 1];
        }
        (*n_tokens) -= (best_merge - 1); // token length decreased by the number of merged tokens minus one
    }

    // add optional EOS token, if desired
    if (eos) tokens[(*n_tokens)++] = EOS;

    free(str_buffer);

}
// END L2E Addition

// ----------------------------------------------------------------------------
// The Sampler, which takes logits and returns a sampled token
// sampling can be done in a few ways: greedy argmax, sampling, top-p sampling

typedef struct {
    float prob;
    int index;
} ProbIndex; // struct used when sorting probabilities during top-p sampling

typedef struct {
    int vocab_size;
    ProbIndex* probindex; // buffer used in top-p sampling
    float temperature;
    float topp;
    unsigned long long rng_state;
} Sampler;

int sample_argmax(float* probabilities, int n) {
    // return the index that has the highest probability
    int max_i = 0;
    float max_p = probabilities[0];
    for (int i = 1; i < n; i++) {
        if (probabilities[i] > max_p) {
            max_i = i;
            max_p = probabilities[i];
        }
    }
    return max_i;
}

int sample_mult(float* probabilities, int n, float coin) {
    // sample index from probabilities (they must sum to 1!)
    // coin is a random number in [0, 1), usually from random_f32()
    float cdf = 0.0f;
    for (int i = 0; i < n; i++) {
        cdf += probabilities[i];
        if (coin < cdf) {
            return i;
        }
    }
    return n - 1; // in case of rounding errors
}

int compare(const void* a, const void* b) {
    ProbIndex* a_ = (ProbIndex*) a;
    ProbIndex* b_ = (ProbIndex*) b;
    if (a_->prob > b_->prob) return -1;
    if (a_->prob < b_->prob) return 1;
    return 0;
}

int sample_topp(float* probabilities, int n, float topp, ProbIndex* probindex, float coin) {
    // top-p sampling (or "nucleus sampling") samples from the smallest set of
    // tokens that exceed probability topp. This way we never sample tokens that
    // have very low probabilities and are less likely to go "off the rails".
    // coin is a random number in [0, 1), usually from random_f32()

    int n0 = 0;
    // quicksort indices in descending order of probabilities
    // values smaller than (1 - topp) / (n - 1) cannot be part of the result
    // so for efficiency we crop these out as candidates before sorting
    const float cutoff = (1.0f - topp) / (n - 1);
    for (int i = 0; i < n; i++) {
        if (probabilities[i] >= cutoff) {
            probindex[n0].index = i;
            probindex[n0].prob = probabilities[i];
            n0++;
        }
    }
    qsort(probindex, n0, sizeof(ProbIndex), compare);

    // truncate the list where cumulative probability exceeds topp
    float cumulative_prob = 0.0f;
    int last_idx = n0 - 1; // in case of rounding errors consider all elements
    for (int i = 0; i < n0; i++) {
        cumulative_prob += probindex[i].prob;
        if (cumulative_prob > topp) {
            last_idx = i;
            break; // we've exceeded topp by including last_idx
        }
    }

    // sample from the truncated list
    float r = coin * cumulative_prob;
    float cdf = 0.0f;
    for (int i = 0; i <= last_idx; i++) {
        cdf += probindex[i].prob;
        if (r < cdf) {
            return probindex[i].index;
        }
    }
    return probindex[last_idx].index; // in case of rounding errors
}

void build_sampler(Sampler* sampler, int vocab_size, float temperature, float topp, unsigned long long rng_seed) {
    sampler->vocab_size = vocab_size;
    sampler->temperature = temperature;
    sampler->topp = topp;
    sampler->rng_state = rng_seed;
    // buffer only used with nucleus sampling; may not need but it's ~small
    sampler->probindex = malloc(sampler->vocab_size * sizeof(ProbIndex));
}

void free_sampler(Sampler* sampler) {
    free(sampler->probindex);
}

unsigned int random_u32(unsigned long long *state) {
    // xorshift rng: https://en.wikipedia.org/wiki/Xorshift#xorshift.2A
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return (*state * 0x2545F4914F6CDD1Dull) >> 32;
}
float random_f32(unsigned long long *state) { // random float32 in [0,1)
    return (random_u32(state) >> 8) / 16777216.0f;
}

int sample(Sampler* sampler, float* logits) {
    // sample the token given the logits and some hyperparameters
    int next;
    if (sampler->temperature == 0.0f) {
        // greedy argmax sampling: take the token with the highest probability
        next = sample_argmax(logits, sampler->vocab_size);
    } else {
        // apply the temperature to the logits
        for (int q=0; q<sampler->vocab_size; q++) { logits[q] /= sampler->temperature; }
        // apply softmax to the logits to get the probabilities for next token
        softmax(logits, sampler->vocab_size);
        // flip a (float) coin (this is our source of entropy for sampling)
        float coin = random_f32(&sampler->rng_state);
        // we sample from this distribution to get the next token
        if (sampler->topp <= 0 || sampler->topp >= 1) {
            // simply sample from the predicted probability distribution
            next = sample_mult(logits, sampler->vocab_size, coin);
        } else {
            // top-p (nucleus) sampling, clamping the least likely tokens to zero
            next = sample_topp(logits, sampler->vocab_size, sampler->topp, sampler->probindex, coin);
        }
    }
    return next;
}

// ----------------------------------------------------------------------------
// utilities: time

long time_in_ms() {
    // return time in milliseconds, for benchmarking the model speed
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

// ----------------------------------------------------------------------------
// generation loop

void generate(Transformer *transformer, Tokenizer *tokenizer, Sampler *sampler, char *prompt, int steps) {
    char *empty_prompt = "";
    if (prompt == NULL) { prompt = empty_prompt; }

    // encode the (string) prompt into tokens sequence
    int num_prompt_tokens = 0;
    int* prompt_tokens = (int*)malloc((strlen(prompt)+3) * sizeof(int)); // +3 for '\0', ?BOS, ?EOS
    encode(tokenizer, prompt, 1, 0, prompt_tokens, &num_prompt_tokens);
    if (num_prompt_tokens < 1) {
        fprintf(stderr, "something is wrong, expected at least 1 prompt token\n");
        exit(EXIT_FAILURE);
    }

    // start the main loop
    long start = 0;  // used to time our code, only initialized after first iteration
    int next;        // will store the next token in the sequence
    int token = prompt_tokens[0]; // kick off with the first token in the prompt
    int pos = 0;     // position in the sequence
// L2E Addition
    int bufferflush = 1; // token counter for flushing buffer
    static char outbuff[4096 * (6 + 2)] ; // buffersize is context length * average size of subwords + margin
    
    // Todo: we can do buffering without setvbuff, implement that
    // setvbuf is used to buffer output into outbuff instead of flushing to screen directly
    if (setvbuf(stdout, outbuff, _IOFBF, sizeof(outbuff)) != 0) {
    puts("Error: Buffer allocation!"); exit(EXIT_FAILURE);
    }
// END L2E Addition
    while (pos < steps) {

        // forward the transformer to get logits for the next token
        float* logits = forward(transformer, token, pos);

        // advance the state machine
        if (pos < num_prompt_tokens - 1) {
            // if we are still processing the input prompt, force the next prompt token
            next = prompt_tokens[pos + 1];
        } else {
            // otherwise sample the next token from the logits
            next = sample(sampler, logits);
        }
        pos++;
        
// L2E Addition
        // data-dependent terminating condition: the BOS token delimits sequences
        if (next == BOS) { break; }
// END L2E Addition

        // print the token as string, decode it with the Tokenizer object
        char* piece = decode(tokenizer, token, next);
        safe_printf(piece); // same as printf("%s", piece), but skips "unsafe" bytes
// L2E Addition
        if (bufferflush==pos) { fflush(stdout); bufferflush+=buffertokens; } 
// END L2E Addition
        token = next;

        // init the timer here because the first iteration can be slower
        if (start == 0) { start = time_in_ms(); }
    }
    printf("\n");
// L2E Addition
    fflush(stdout); // This could be in the if next break, and the print new line prepended to achieved tok/s
// END L2E Addition
    // report achieved tok/s (pos-1 because the timer starts after first iteration)
    if (pos > 1) {
        long end = time_in_ms();
// L2E Addition
        if(stats){ fprintf(stderr, "achieved tok/s: %f\n", (pos-1) / (double)(end-start)*1000); }
// END L2E Addition
    }

    free(prompt_tokens);
}

void read_stdin(const char* guide, char* buffer, size_t bufsize) {
    // read a line from stdin, up to but not including \n
    printf("%s", guide);
    if (fgets(buffer, bufsize, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0'; // strip newline
        }
    }
}

// ----------------------------------------------------------------------------
// chat loop
// I manually inspected the tokens for a few chat conversations compared to
// python reference and that seemed ok, but this was not thoroughly tested and
// is not safely implemented, it's more a proof of concept atm.

void chat(Transformer *transformer, Tokenizer *tokenizer, Sampler *sampler,
          char *cli_user_prompt, char *cli_system_prompt, int steps) {

    // buffers for reading the system prompt and user prompt from stdin
    // you'll notice they are soomewhat haphazardly and unsafely set atm
// L2E Addition
    char system_prompt[512];
    char user_prompt[512];
    char rendered_prompt[2048];
    int num_prompt_tokens = 0;
    int* prompt_tokens = (int*)malloc(2048 * sizeof(int));
// END L2E Addition
    int user_idx;

    // start the main loop
    int8_t user_turn = 1; // user starts
    int next;        // will store the next token in the sequence
    int token;       // stores the current token to feed into the transformer
// L2E Addition
    /* System and user prompt templates for llama 2 and llama 3
    Llama 2:
    System:
    [INST] <<SYS>>\n%s\n<</SYS>>\n\n%s [/INST]
    User:
    [INST] %s [/INST]

    Llama 3:
    System:
    <|begin_of_text|><|start_header_id|>system<|end_header_id|>\n\n%s<|eot_id|>
    User:
    <|start_header_id|>user<|end_header_id|>\n\n%s<|eot_id|>\n
    Assistant: (Starts Generating)
    <|start_header_id|>assistant<|end_header_id|>\n\n
    */
    if (llamaver == 3) {
        BOS = 128000; // 128000 = <|begin_of_text|>
        EOS = 128009; // 128009 = <|eot_id|> , 128001 = <|end_of_text|>
        strcpy(system_template, "<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n\n%s<|eot_id|>");
        strcpy(user_template, "<|start_header_id|>user<|end_header_id|>\n\n%s<|eot_id|>\n<|start_header_id|>assistant<|end_header_id|>\n\n");
    } else { 
        int prev_token; 
        strcpy(system_template,"[INST] <<SYS>>\n%s\n<</SYS>>\n\n%s [/INST]");
        strcpy(user_template, "[INST] %s [/INST]");
    }
// END L2E Addition
    int pos = 0;     // position in the sequence
    while (pos < steps) {

        // when it is the user's turn to contribute tokens to the dialog...
        if (user_turn) {
            // get the (optional) system prompt at position 0
            if (pos == 0) {
                // at position 0, the user can also contribute a system prompt
                if (cli_system_prompt == NULL) {
                    // system prompt was not passed in, attempt to get it from stdin
                    read_stdin("Enter system prompt (optional): ", system_prompt, sizeof(system_prompt));
                } else {
                    // system prompt was passed in, use it
                    strcpy(system_prompt, cli_system_prompt);
                }
            }
            // get the user prompt
            if (pos == 0 && cli_user_prompt != NULL) {
                // user prompt for position 0 was passed in, use it
                strcpy(user_prompt, cli_user_prompt);
            } else {
                // otherwise get user prompt from stdin
                read_stdin("User: ", user_prompt, sizeof(user_prompt));
            }
// L2E Addition
            // render user/system prompts into the Llama 2 Chat schema
            if (pos == 0 && system_prompt[0] != '\0') {
                sprintf(rendered_prompt, system_template, system_prompt, user_prompt);
            } else {
                sprintf(rendered_prompt, user_template, user_prompt);
            }
// END L2E Addition
            // encode the rendered prompt into tokens
            encode(tokenizer, rendered_prompt, 1, 0, prompt_tokens, &num_prompt_tokens);
            user_idx = 0; // reset the user index
            user_turn = 0;
            printf("Assistant: ");
        }

        // determine the token to pass into the transformer next
        if (user_idx < num_prompt_tokens) {
            // if we are still processing the input prompt, force the next prompt token
            token = prompt_tokens[user_idx++];
        } else {
            // otherwise use the next token sampled from previous turn
            token = next;
        }
// L2E Addition        
        // EOS token ends the Assistant turn
        if (token == EOS) { user_turn = 1; }
// End L2E Addition

        // forward the transformer to get logits for the next token
        float* logits = forward(transformer, token, pos);
        next = sample(sampler, logits);
        pos++;
// L2E Addition        
        if (user_idx >= num_prompt_tokens && next != EOS) {
            // the Assistant is responding, so print its output
            char* piece = decode(tokenizer, token, next);
            safe_printf(piece); // same as printf("%s", piece), but skips "unsafe" bytes
            fflush(stdout);
        }
        if (next == EOS) { printf("\n"); }
    }
// End L2E Addition
    printf("\n");
    free(prompt_tokens);
}

// L2E Addition
// ----------------------------------------------------------------------------
// LLama 2 Everywhere read prompt utility function

#if defined(COSMO_ZIP) || defined(INC_BIN) || defined(STRLIT)
void inprompt(char *lprompt) // Handle prompts
{
    fgets(lprompt, 1024, stdin);
    lprompt[strcspn(lprompt, "\n")] = '\0';
}
#endif

// ----------------------------------------------------------------------------
// Main Section
// END L2E Addition

// ----------------------------------------------------------------------------
// CLI, include only if not testing
#ifndef TESTING

void error_usage() {
    fprintf(stderr, "Usage:   run <checkpoint> [options]\n");
    fprintf(stderr, "Example: run model.bin -n 256 -i \"Once upon a time\"\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t <float>  temperature in [0,inf], default 1.0\n");
    fprintf(stderr, "  -p <float>  p value in top-p (nucleus) sampling in [0,1] default 0.9\n");
    fprintf(stderr, "  -s <int>    random seed, default time(NULL)\n");
    fprintf(stderr, "  -n <int>    number of steps to run for, default 256. 0 = max_seq_len\n");
    fprintf(stderr, "  -i <string> input prompt\n");
    fprintf(stderr, "  -z <string> optional path to custom tokenizer\n");
    fprintf(stderr, "  -m <string> mode: generate|chat, default: generate\n");
    fprintf(stderr, "  -y <string> (optional) system prompt in chat mode\n");
// L2E Addition
    fprintf(stderr, "  -b <int>    number of tokens to buffer, default 1. 0 = max_seq_len\n");
    fprintf(stderr, "  -x <int>    extended info / stats, default 1 = on. 0 = off\n");
    fprintf(stderr, "  -l <int>    llama version / default 2 = llama2. 3 = llama3\n");
// END L2E Addition
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
// L2E Addition
#ifdef OPENMP
    int num_threads = omp_get_num_procs(); // get the number of CPU cores
    omp_set_num_threads(num_threads); // set the number of threads to use for parallel regions
    int num_levels = omp_get_supported_active_levels(); // get maximum number of nested parallel regions supported
    omp_set_max_active_levels(num_levels); // set to maximum supported parallel regions
#endif
// END L2E Addition
    // default parameters
    char *checkpoint_path = NULL;  // e.g. out/model.bin
    char *tokenizer_path = "tokenizer.bin";
    float temperature = 1.0f;   // 0.0 = greedy deterministic. 1.0 = original. don't set higher
    float topp = 0.9f;          // top-p in nucleus sampling. 1.0 = off. 0.9 works well, but slower
    int steps = 256;            // number of steps to run for
    char *prompt = NULL;        // prompt string
    unsigned long long rng_seed = 0; // seed rng with time by default
    char *mode = "generate";    // generate|chat
    char *system_prompt = NULL; // the (optional) system prompt to use in chat mode
// L2E Addition
    #if defined(COSMO_ZIP) || defined(INC_BIN) || defined(STRLIT) // special case for embedded models
    // we read the embedded checkpoint from within the executable
    #ifdef UNIK
    printf("\n*** GURU UNMEDITATION :: BOOT > LLAMA HAS AWAKENED ***\n\n");
    #endif
    #if defined(COSMO_ZIP)
    checkpoint_path = "/zip/out/model.bin";
    tokenizer_path = "/zip/tokenizer.bin";
    #elif defined(INC_BIN) || defined(STRLIT)
    checkpoint_path = emb_Model_data;
    tokenizer_path = emb_Tokenizer_data;
    #endif
    buffertokens=8;
    #ifdef LLOOP
    stats = LOOPSTATUS;
    while(1) { // start of loop
    #endif
    prompt=(char*)malloc(1024);
    printf("\n" DEFTOSTR(OSPROMPT)" ");
    fflush(stdout); 
    inprompt(prompt); // read prompt
    if (!strcmp(prompt, "exit")) { exit(0);} // Exit when prompt contains exit
    #else
// END L2E Addition
    // poor man's C argparse so we can override the defaults above from the command line
    if (argc >= 2) { checkpoint_path = argv[1]; } else { error_usage(); }
    for (int i = 2; i < argc; i+=2) {
        // do some basic validation
        if (i + 1 >= argc) { error_usage(); } // must have arg after flag
        if (argv[i][0] != '-') { error_usage(); } // must start with dash
        if (strlen(argv[i]) != 2) { error_usage(); } // must be -x (one dash, one letter)
        // read in the args
        if (argv[i][1] == 't') { temperature = atof(argv[i + 1]); }
        else if (argv[i][1] == 'p') { topp = atof(argv[i + 1]); }
        else if (argv[i][1] == 's') { rng_seed = atoi(argv[i + 1]); }
        else if (argv[i][1] == 'n') { steps = atoi(argv[i + 1]); }
        else if (argv[i][1] == 'i') { prompt = argv[i + 1]; }
        else if (argv[i][1] == 'z') { tokenizer_path = argv[i + 1]; }
        else if (argv[i][1] == 'm') { mode = argv[i + 1]; }
        else if (argv[i][1] == 'y') { system_prompt = argv[i + 1]; }
// L2E Addition
        else if (argv[i][1] == 'b') { buffertokens = atoi(argv[i + 1]); }
        else if (argv[i][1] == 'x') { stats = atoi(argv[i + 1]); }
        else if (argv[i][1] == 'l') { llamaver = atoi(argv[i + 1]); }
// END L2E Addition
        else { error_usage(); }
    }
    if (llamaver == 3){ rope_tf = 500000.0; }
// L2E Addition
    #endif
// END L2E Addition
    // parameter validation/overrides
    if (rng_seed <= 0) rng_seed = (unsigned int)time(NULL);
    if (temperature < 0.0) temperature = 0.0;
    if (topp < 0.0 || 1.0 < topp) topp = 0.9;
    if (steps < 0) steps = 0;

    // build the Transformer via the model .bin file
    Transformer transformer;
    build_transformer(&transformer, checkpoint_path);
    if (steps == 0 || steps > transformer.config.seq_len) steps = transformer.config.seq_len; // override to ~max length

    // build the Tokenizer via the tokenizer .bin file
    Tokenizer tokenizer;
    build_tokenizer(&tokenizer, tokenizer_path, transformer.config.vocab_size);

    // build the Sampler
    Sampler sampler;
    build_sampler(&sampler, transformer.config.vocab_size, temperature, topp, rng_seed);

    // run!
    if (strcmp(mode, "generate") == 0) {
        generate(&transformer, &tokenizer, &sampler, prompt, steps);
    } else if (strcmp(mode, "chat") == 0) {
        chat(&transformer, &tokenizer, &sampler, prompt, system_prompt, steps);
    } else {
        fprintf(stderr, "unknown mode: %s\n", mode);
        error_usage();
    }

    // memory and file handles cleanup
    free_sampler(&sampler);
    free_tokenizer(&tokenizer);
    free_transformer(&transformer);
// L2E Addition
    #if defined(COSMO_ZIP) || defined(INC_BIN) || defined(STRLIT)
    #ifdef LLOOP
    printf("\n");
    } // end of loop
    #endif
    #endif    
// END L2E Addition
    return 0;
}
#endif
