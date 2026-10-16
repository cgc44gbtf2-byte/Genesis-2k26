#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern OBJ_DATA *get_page(OBJ_DATA *,long);
extern void fread_char(CHAR_DATA *,FILE *);
extern char *target_name;
static void input(CHAR_DATA *ch,void (*fn)(CHAR_DATA *,char *),const char *s)
{ char b[MAX_INPUT_LENGTH]; assert(strlen(s)<sizeof(b)); strcpy(b,s); fn(ch,b); }
static void purge(CHAR_DATA *ch) { while(ch->affected) affect_remove(ch,ch->affected); }
int main(void)
{
    CHAR_DATA ch,v;
    PC_DATA pc;
    ROOM_INDEX_DATA room;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    OBJ_INDEX_DATA proto;
    OBJ_DATA *book,*pen,*page;
    DESCRIPTOR_DATA desc;
    char output[65536];
    FILE *fp;
    long mana,hp,armor,primal;
    int i,j,mask=0;
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&ch,0,sizeof(ch)); memset(&v,0,sizeof(v)); memset(&pc,0,sizeof(pc));
    memset(&room,0,sizeof(room)); memset(&area,0,sizeof(area)); memset(&mob,0,sizeof(mob));
    memset(&proto,0,sizeof(proto)); memset(&desc,0,sizeof(desc)); memset(output,0,sizeof(output));
    ch.pcdata=&pc; ch.name=ch.short_descr="Booktester"; ch.class=CLASS_SORCERER; ch.level=LEV_SOR;
    ch.morph=ch.description=ch.long_descr=ch.clan=ch.lord=ch.contingency=str_empty;
    ch.hit=ch.max_hit=ch.mana=ch.max_mana=1000000; ch.position=POS_STANDING;
    ch.primal=123; for(i=0;i<5;++i) ch.stats[i]=5;
    room.name="a room"; room.area=&area; room.light=1; char_to_room(&ch,&room);
    char_list=&ch; desc.character=&ch; desc.outbuf=output; desc.outsize=sizeof(output); ch.desc=&desc;
    assert(!sorcerer_starter_kit(&ch) && !pc.sorcerer_starter && !ch.carrying);
    proto.vnum=OBJ_VNUM_PROTOPLASM; proto.name=proto.short_descr=proto.description=str_empty;
    proto.item_type=ITEM_TRASH; proto.wear_flags=ITEM_TAKE;
    obj_index_hash[proto.vnum%MAX_KEY_HASH]=&proto;
    primal=ch.primal; pc.colors[BLUE_MAGIC]=3;
    assert(sorcerer_starter_kit(&ch) && pc.sorcerer_starter && ch.primal==primal);
    book=get_obj_carry(&ch,"primer"); pen=get_obj_carry(&ch,"pen");
    assert(book && pen && book->item_type==ITEM_BOOK && pen->item_type==ITEM_PEN);
    assert(!IS_SET(book->spectype,ADV_CHAOS_BOOK));
    assert(book->value[3]==5 && pc.colors[BLUE_MAGIC]==3 && pc.colors[GREEN_MAGIC]==1);
    for(i=1;i<=5;++i) assert(get_page(book,i));
    assert(strstr(get_page(book,1)->chpoweruse,"primer 2"));
    assert(!sorcerer_starter_kit(&ch));
    equip_char(&ch,book,WEAR_HOLD);
    mana=ch.mana; input(&ch,do_chant,"self");
    assert(ch.mana==mana && strstr(output,"reading page, not a spell"));
    input(&ch,do_turn,"primer 3"); hp=ch.hit=500000; mana=ch.mana;
    input(&ch,do_chant,"self"); assert(ch.hit>hp && ch.mana==mana-400);
    input(&ch,do_turn,"primer 4"); armor=ch.armor; mana=ch.mana;
    input(&ch,do_chant,"self"); assert(ch.armor==armor+10 && ch.mana==mana-1560);
    mana=ch.mana; input(&ch,do_chant,"self"); assert(ch.armor==armor+10 && ch.mana==mana);
    purge(&ch); assert(ch.armor==armor);
    v.name=v.short_descr="Enemy"; v.morph=v.long_descr=v.description=v.clan=v.lord=str_empty;
    v.level=20; v.act=ACT_IS_NPC; v.pIndexData=&mob; v.position=POS_STANDING;
    v.hit=v.max_hit=1000000; for(i=0;i<5;++i) v.stats[i]=5;
    char_to_room(&v,&room); ch.next=&v;
    input(&ch,do_turn,"primer 2"); hp=v.hit; mana=ch.mana;
    input(&ch,do_chant,"Enemy"); assert(v.hit<hp && ch.mana==mana-200);
    stop_fighting(&ch,TRUE);
    /* Execute the tutorial's exact recipe using the real writing/binding engine. */
    unequip_char(&ch,book); equip_char(&ch,pen,WEAR_HOLD);
    input(&ch,do_sorccreate,"page");
    input(&ch,do_write,"page title My Spark");
    input(&ch,do_write,"page line start.damage.spell");
    input(&ch,do_write,"page line message.1: sparks");
    input(&ch,do_write,"page line message.2: You send sparks toward $N.");
    input(&ch,do_write,"page line message.3: $n sends sparks toward $N.");
    input(&ch,do_write,"page line end.yellow");
    page=get_obj_carry(&ch,"page");
    assert(page && IS_SET(page->spectype,ADV_FINISHED) && !IS_SET(page->spectype,ADV_FAILED));
    unequip_char(&ch,pen); target_name="page primer";
    spell_insert_page(skill_lookup("insert page"),LEV_SOR,&ch,NULL);
    assert(book->value[3]==6 && get_page(book,6)==page);
    equip_char(&ch,book,WEAR_HOLD); hp=v.hit; mana=ch.mana;
    input(&ch,do_chant,"Enemy"); assert(v.hit<hp && ch.mana==mana-200);
    stop_fighting(&ch,TRUE); target_name="";
    SET_BIT(book->spectype,ADV_CHAOS_BOOK);
    for(i=0;i<6;++i) pc.colors[i]=1;
    input(&ch,do_turn,"primer 4");
    for(i=0;i<80;++i) {
        ch.mana=1000000; armor=ch.armor; input(&ch,do_chant,"self");
        assert(ch.armor==armor+10 && ch.mana==1000000-1560);
        for(j=0;j<6;++j) {
            const char *names[]={"purple sorcery","red sorcery","blue sorcery",
                "green sorcery","yellow sorcery","black sorcery"};
            if(is_affected(&ch,skill_lookup(names[j]))) mask|=1<<j;
        }
        purge(&ch);
    }
    assert(mask==63 && get_page(book,4)->toughness==GREEN_MAGIC);
    input(&ch,do_turn,"primer 3");
    for(i=0;i<30;++i) {
        ch.hit=500000; ch.mana=1000000; input(&ch,do_chant,"self");
        assert(ch.hit>500000 && ch.hit<ch.max_hit);
    }
    input(&ch,do_turn,"primer 2");
    for(i=0;i<30;++i) {
        v.hit=500000; ch.mana=1000000; input(&ch,do_chant,"Enemy");
        assert(v.hit<500000);
    }
    stop_fighting(&ch,TRUE);
    /* Saved flags parse without consuming the following player field. */
    pc.sorcerer_starter=pc.sorcerer_chaos=0;
    fp=tmpfile(); assert(fp); fputs("SorcStarter 1\nSorcStyle 1\nExp 9876\nEnd\n",fp); rewind(fp);
    fread_char(&ch,fp); fclose(fp);
    assert(pc.sorcerer_starter==1 && pc.sorcerer_chaos==1 && ch.exp==9876);
    assert(!sorcerer_starter_kit(&ch));
    unequip_char(&ch,book); while(ch.carrying) extract_obj(ch.carrying);
    /* Existing unclaimed Chaos Sorcerers get all six basic colours, not reset mastery. */
    pc.sorcerer_starter=0; pc.colors[RED_MAGIC]=4;
    assert(sorcerer_starter_kit(&ch)); book=get_obj_carry(&ch,"primer");
    assert(IS_SET(book->spectype,ADV_CHAOS_BOOK) && pc.colors[RED_MAGIC]==4);
    for(i=0;i<6;++i) assert(pc.colors[i]>=1);
    page=get_page(book,4); assert(!IS_SET(page->spectype,ADV_REVERSED) && page->level==2);
    primal=ch.primal; input(&ch,do_sorccreate,"book"); book=get_obj_carry(&ch,"book");
    assert(book && IS_SET(book->spectype,ADV_CHAOS_BOOK) && ch.primal==primal-10);
    while(ch.carrying) extract_obj(ch.carrying);
    puts("Book mage primer: five readable pages, three actual spell effects, exact costs, preserved mastery and one-time claims passed");
    puts("Chaos book: same blessing page rolled all six colours; 30 healing and 30 attack rolls preserved their role; save flags passed");
    return 0;
}
