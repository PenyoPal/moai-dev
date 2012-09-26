#include <moaiext-sphinx/MOAISphinx.h>
#include <pocketsphinx.h>
#include <stdio.h>

int MOAISphinx::_initialize_sphinx(lua_State* L) {
	MOAI_LUA_SETUP(MOAISphinx, "U")
	
	cmd_ln_t *config;
	
	config = cmd_ln_init(NULL, ps_args(), TRUE,
						 "-hmm", MODEL_DIR "tdt_sc_8kadapt",
						 "-lm", MODEL_DIR "penyo.lm.dmp",
						 "-dict", MODEL_DIR "penyo.dic",
						 NULL);
	if (config == NULL) {
		perror("Failed to create config");
		return -1;
	}
	
	self->ps = ps_init(config);
	if (self->ps == NULL) {
		perror("Failed to initialize decoder");
		return -1;
	}
	return 0;
}

int MOAISphinx::_analyze_utterance(lua_State* L) {
	MOAI_LUA_SETUP(MOAISphinx, "UNT")
	
	u32 data_size = state.GetValue<u32>(2, 1);
	printf("Data size %d\n", data_size);
	if (data_size == 0) {
		for (int i = 0; i < 3; i++) {
			lua_pushlstring ( state, "?", 1 );
		}
		return 3;
	}
	int itr = state.PushTableItr(3);
	int16 *data;
	u32 idx = 0;
	data = (int16*)malloc(data_size * sizeof(int16));
	
	for (; state.TableItrNext(itr) && idx < data_size; ++idx) {
		int val = state.GetValue<int>(-1, 0); 
		data[idx] = val;
	}
	
	ps_start_utt(self->ps, NULL);
	int rv = ps_process_raw(self->ps, data, data_size, 0, 1);
	ps_end_utt(self->ps);
	free(data);
	if (rv < 0) {
		perror("Error decoding input audio");
		return -1;
	}
	
	const char *hyps[3] = { NULL, NULL, NULL };
	const char *uttid;
	int32 scores[3] = { 0, 0, 0 };
	hyps[0] = ps_get_hyp(self->ps, scores, &uttid);
	
	self->nbest = ps_nbest(self->ps, 0, -1, NULL, NULL);
	if (self->nbest == NULL) {
		perror("Error creating hypothesis iterator");
		return -1;
	}
	
	ps_nbest_next(self->nbest); ps_nbest_next(self->nbest);
	for (int i = 1; i < 3 && self->nbest; i++, ps_nbest_next(self->nbest)) {
		hyps[i] = ps_nbest_hyp(self->nbest, scores + i);
		if (hyps[i] == NULL) break;
	}
	for (int i = 2; i >= 0; i--) {
		if (hyps[i]) {
			lua_pushlstring ( state, hyps[i], strlen(hyps[i]) );
		} else {
			lua_pushlstring ( state, "?", 1 ) ;
		}
	}
		
	return 3;
}

MOAISphinx::MOAISphinx() {
	RTTI_BEGIN
	RTTI_EXTEND ( MOAILuaObject )
	RTTI_END
}

MOAISphinx::~MOAISphinx() {
	ps_free(ps);
}

void MOAISphinx::RegisterLuaClass(MOAILuaState& state) {
	luaL_Reg regTable[] = {
		{ "analyze_utterance", _analyze_utterance },
		{ "initialize_sphinx", _initialize_sphinx },
		{ NULL, NULL }
	};
	
	luaL_register ( state, 0, regTable );
}
