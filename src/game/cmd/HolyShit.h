#ifndef GAME_CMD_HOLYSHIT_H
#define GAME_CMD_HOLYSHIT_H

///////////////////////////////////////////////////////////////////////////////

class HolyShit : public AbstractBuiltin
{
protected:
    PostAction doExecute( Context& );

public:
    HolyShit();
    ~HolyShit();
};

///////////////////////////////////////////////////////////////////////////////

#endif // GAME_CMD_HOLYSHIT_H
