#ifndef GAME_CMD_SNIPERWAR_H
#define GAME_CMD_SNIPERWAR_H

///////////////////////////////////////////////////////////////////////////////

class SniperWar : public AbstractBuiltin
{
protected:
    PostAction doExecute( Context& );

public:
    SniperWar();
    ~SniperWar();
};

///////////////////////////////////////////////////////////////////////////////

#endif // GAME_CMD_SNIPERWAR_H
