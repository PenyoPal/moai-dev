#ifndef MOAISPHINX_H
#define MOAISPHINX_H

#include <moaicore/moaicore.h>
#include <pocketsphinx.h>

#define MODEL_DIR "../penyo_training/"

class MOAISphinx : public MOAIGlobalClass< MOAISphinx, MOAILuaObject > {

  private:
    ps_decoder_t *ps;
    ps_nbest_t *nbest;
    static int _initialize_sphinx(lua_State* L);
    static int _analyze_utterance(lua_State* L);

  public:
	DECL_LUA_SINGLETON(MOAISphinx)
	
    MOAISphinx();
    ~MOAISphinx();

    void RegisterLuaClass(MOAILuaState& state);
};

#endif
