/* Genesis-native starter pages, compiled with Sorcerer's existing book engine. */
static long sorcerer_page_color(CHAR_DATA *ch,OBJ_DATA *book,OBJ_DATA *page)
{
    static const char *names[]={"purple","red","blue","green","yellow","black"};
    char buf[128];
    long color=page->toughness;
    if (IS_SET(book->spectype,ADV_CHAOS_BOOK)) {
        color=number_range(0,5);
        format_buffer(buf,sizeof(buf),"The page's chaos magic settles on %s.\n\r",names[color]);
        stc(buf,ch);
    }
    return color;
}

static OBJ_DATA *sorcerer_starter_item(OBJ_INDEX_DATA *proto,CHAR_DATA *ch,
    long type,const char *name,const char *title)
{
    OBJ_DATA *obj=create_object(proto,0);
    memset(obj->value,0,sizeof(obj->value));
    obj->item_type=type; obj->spectype=0; obj->specpower=0;
    obj->level=0; obj->toughness=0; obj->cost=0; obj->weight=1; obj->timer=0;
    obj->wear_flags=ITEM_TAKE | (type==ITEM_PAGE ? 0 : ITEM_WEAR_HOLD);
    free_string(obj->name); obj->name=str_dup(name);
    free_string(obj->short_descr); obj->short_descr=str_dup(title);
    free_string(obj->description); obj->description=str_dup("A beginner's magical tool lies here.");
    free_string(obj->victpoweruse); obj->victpoweruse=str_dup(title);
    free_string(obj->questmaker); obj->questmaker=str_dup(ch->name);
    free_string(obj->questowner); obj->questowner=str_dup(ch->name);
    return obj;
}

static void sorcerer_starter_page(OBJ_INDEX_DATA *proto,CHAR_DATA *ch,OBJ_DATA *book,
    int number,const char *title,const char *text,long type,long color,
    const char *one,const char *two,const char *three)
{
    OBJ_DATA *page=sorcerer_starter_item(proto,ch,ITEM_PAGE,"lessonpage page",title);
    page->value[0]=number; page->specpower=number+1; page->toughness=color;
    free_string(page->chpoweruse); page->chpoweruse=str_dup(text);
    if (type) {
        page->spectype=ADV_STARTED|ADV_FINISHED|ADV_MESSAGE_1|ADV_MESSAGE_2|ADV_MESSAGE_3|type;
        free_string(page->chpoweroff); page->chpoweroff=str_dup(one);
        free_string(page->victpoweron); page->victpoweron=str_dup(two);
        free_string(page->victpoweroff); page->victpoweroff=str_dup(three);
        if (type==ADV_AFFECT) {
            page->value[1]=ADV_AC; page->level=2;
        }
    }
    obj_to_obj(page,book);
}

bool sorcerer_starter_kit(CHAR_DATA *ch)
{
    OBJ_INDEX_DATA *proto;
    OBJ_DATA *book,*pen;
    int i;
    bool chaos;
    if (!ch || IS_NPC(ch) || !ch->pcdata || !IS_SORCERER(ch) || !ch->in_room) return FALSE;
    if (ch->pcdata->sorcerer_starter) {
        stc("You have already received your one-time Sorcerer starter kit.\n\r",ch); return FALSE;
    }
    if (ch->fighting || ch->pcdata->chobj || IS_MORE(ch,MORE_HEAD)) {
        stc("Return to normal form out of combat, then use sorccreate starter.\n\r",ch);
        return FALSE;
    }
    proto=get_obj_index(OBJ_VNUM_PROTOPLASM);
    if (!proto) {
        stc("The starter template is unavailable; an administrator must restore it. Try sorccreate starter later.\n\r",ch);
        return FALSE;
    }
    chaos=ch->pcdata->sorcerer_chaos==1;
    book=sorcerer_starter_item(proto,ch,ITEM_BOOK,"primer spellbook book starter",
        chaos ? "the Chaos Book Mage's primer" : "the Book Mage's primer");
    book->weight=2; book->value[0]=1000; book->value[1]=CONT_CLOSEABLE;
    book->value[2]=1; book->value[3]=5;
    if (chaos) SET_BIT(book->spectype,ADV_CHAOS_BOOK);
    sorcerer_starter_page(proto,ch,book,1,"Getting started",
        "Hold primer. It starts open. Read primer; turn primer 0 shows the index.\n\r"
        "Turn primer 2; chant <enemy> casts Spark. Normal cost: 200 mana.\n\r"
        "Turn primer 3; chant self casts Mend (400 mana).\n\r"
        "Turn primer 4; chant self casts Minor Ward (1560 mana, two ticks).\n\r"
        "Ward raises Genesis armor, improving defense. Same-color wards do not stack.\n\r"
        "Turn primer 5; read primer for a working spell-writing recipe.\n\r"
        "Colors shows your six magic skills; colors <color> buys training with primal.\n\r"
        "Blue, green and yellow start at least at rank 1; Chaos mages start all six at 1.\n\r"
        "Chaos books roll a new color per page per chant; the color can repeat.\n\r"
        "The color changes power and damage/healing costs, not a page's basic role.\n\r"
        "Action pages retain their written travel/summoning operation for safety.\n\r"
        "The pen and starter kit are free once per character. Ordinary crafting costs still apply.",
        0,0,"","","");
    sorcerer_starter_page(proto,ch,book,2,"Spark",
        "Start.damage.spell\n\rmessage.1: sparks\n\rmessage.2: You send sparks toward $N.\n\r"
        "message.3: $n sends sparks toward $N.\n\rend.yellow",
        ADV_DAMAGE,YELLOW_MAGIC,"sparks","You send sparks toward $N.","$n sends sparks toward $N.");
    sorcerer_starter_page(proto,ch,book,3,"Mend",
        "Start.damage.spell\n\rmessage.1: healing light\n\rmessage.2: You weave healing light around $N.\n\r"
        "message.3: $n weaves healing light around $N.\n\rend.blue",
        ADV_DAMAGE,BLUE_MAGIC,"healing light","You weave healing light around $N.","$n weaves healing light around $N.");
    sorcerer_starter_page(proto,ch,book,4,"Minor Ward",
        "Start.affect.spell\n\rapply: armor\n\rduration: 2\n\r"
        "message.1: You weave a protective ward around $N.\n\rmessage.2: A protective ward surrounds you.\n\r"
        "message.3: A protective ward surrounds $N.\n\rend.green",
        ADV_AFFECT,GREEN_MAGIC,"You weave a protective ward around $N.",
        "A protective ward surrounds you.","A protective ward surrounds $N.");
    sorcerer_starter_page(proto,ch,book,5,"Writing your own spells",
        "Remove primer; sorccreate page; hold pen. Your kit already includes a pen.\n\r"
        "Hold pen. Enter each command below on a separate line:\n\r"
        "write page title My Spark\n\rwrite page line start.damage.spell\n\r"
        "write page line message.1: sparks\n\rwrite page line message.2: You send sparks toward $N.\n\r"
        "write page line message.3: $n sends sparks toward $N.\n\rwrite page line end.yellow\n\r"
        "Remove pen. Cast 'insert page' page primer. Hold primer; turn primer 6; chant <enemy>.\n\r"
        "Read your page before binding: finished spells need three messages and end.<color>.\n\r"
        "For a fixed healing spell use end.blue. Ordinary end.random changes native spell behavior!\n\r"
        "A Chaos-marked book instead rerolls the color while preserving the written spell's role.\n\r"
        "For a ward use start.affect.spell, apply: armor, duration: 2, three messages, end.green.\n\r"
        "Do not use next.page while learning: it chains additional spells and costs.\n\r"
        "Sorccreate book and sorccreate pen cost 10 primal each; pages are free.",
        0,0,"","","");
    pen=sorcerer_starter_item(proto,ch,ITEM_PEN,"scribe inscription pen","a beginner's inscription pen");
    obj_to_char(book,ch); obj_to_char(pen,ch);
    for (i=0;i<6;++i)
        if (chaos || i==BLUE_MAGIC || i==GREEN_MAGIC || i==YELLOW_MAGIC)
            ch->pcdata->colors[i]=UMAX(1,ch->pcdata->colors[i]);
    ch->pcdata->sorcerer_starter=1;
    stc("You receive a free primer, three ready-to-chant spells, two instruction pages and a pen.\n\r"
        "Hold primer; read primer. Existing Sorcerers can claim this kit with sorccreate starter.\n\r",ch);
    return TRUE;
}
