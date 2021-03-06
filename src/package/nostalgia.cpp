#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "nostalgia.h"

class MoonSpearSkill: public WeaponSkill{
public:
    MoonSpearSkill():WeaponSkill("moon_spear"){
        events << CardFinished << CardResponsed;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::NotActive)
            return false;

        CardStar card = NULL;
        if(event == CardFinished){
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;

            if(card == player->tag["MoonSpearSlash"].value<CardStar>()){
                card = NULL;
            }
        }else if(event == CardResponsed){
            card = data.value<CardStar>();
            player->tag["MoonSpearSlash"] = data;
        }

        if(card == NULL || !card->isBlack())
            return false;

        room->askForUseCard(player, "slash", "@moon-spear-slash");

        return false;
    }
};

class MoonSpear: public Weapon{
public:
    MoonSpear(Suit suit = Card::Diamond, int number = 12)
        :Weapon(suit, number, 3){
        setObjectName("moon_spear");
        skill = new MoonSpearSkill;
    }
};

NostalgiaPackage::NostalgiaPackage()
    :Package("nostalgia")
{
    type = CardPack;

    Card *moon_spear = new MoonSpear;
    moon_spear->setParent(this);
}

// old yjcm's generals

class NosWuyan: public TriggerSkill{
public:
    NosWuyan():TriggerSkill("noswuyan"){
        events << CardEffect << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.to == effect.from)
            return false;

        if(effect.card->getTypeId() == Card::Trick){
            if(effect.from && effect.from->hasSkill(objectName())){
                LogMessage log;
                log.type = "#WuyanBaD";
                log.from = effect.from;
                log.to << effect.to;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect("wuyan");

                return true;
            }

            if(effect.to->hasSkill(objectName()) && effect.from){
                LogMessage log;
                log.type = "#WuyanGooD";
                log.from = effect.to;
                log.to << effect.from;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect("wuyan");

                return true;
            }
        }

        return false;
    }
};

NosJujianCard::NosJujianCard(){
    once = true;
    mute = true;
}

void NosJujianCard::onEffect(const CardEffectStruct &effect) const{
    int n = subcardsLength();
    effect.to->drawCards(n);
    Room *room = effect.from->getRoom();
    room->playSkillEffect("jujian");

    if(n == 3){
        QSet<Card::CardType> types;

        foreach(int card_id, effect.card->getSubcards()){
            const Card *card = Sanguosha->getCard(card_id);
            types << card->getTypeId();
        }

        if(types.size() == 1){

            LogMessage log;
            log.type = "#JujianRecover";
            log.from = effect.from;
            const Card *card = Sanguosha->getCard(subcards.first());
            log.arg = card->getType();
            room->sendLog(log);

            RecoverStruct recover;
            recover.card = this;
            recover.who = effect.from;
            room->recover(effect.from, recover);
        }
    }
}

class NosJujian: public ViewAsSkill{
public:
    NosJujian():ViewAsSkill("nosjujian"){

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 3;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("NosJujianCard");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        NosJujianCard *card = new NosJujianCard;
        card->addSubcards(cards);
        return card;
    }
};

class EnyuanPattern: public CardPattern{
public:
    virtual bool match(const Player *player, const Card *card) const{
        return !player->hasEquip(card) && card->getSuit() == Card::Heart;
    }

    virtual bool willThrow() const{
        return false;
    }
};

class NosEnyuan: public TriggerSkill{
public:
    NosEnyuan():TriggerSkill("nosenyuan"){
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(event == HpRecover){
            RecoverStruct recover = data.value<RecoverStruct>();
            if(recover.who && recover.who != player){
                recover.who->drawCards(recover.recover);

                LogMessage log;
                log.type = "#EnyuanRecover";
                log.from = player;
                log.to << recover.who;
                log.arg = QString::number(recover.recover);
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect("enyuan", qrand() % 2 + 1);

            }
        }else if(event == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if(source && source != player){
                room->playSkillEffect("enyuan", qrand() % 2 + 3);

                const Card *card = room->askForCard(source, ".enyuan", "@enyuanheart", QVariant(), NonTrigger);
                if(card){
                    room->showCard(source, card->getEffectiveId());
                    player->obtainCard(card);
                }else{
                    room->loseHp(source);
                }
            }
        }

        return false;
    }
};

NosXuanhuoCard::NosXuanhuoCard(){
    once = true;
    will_throw = false;
    mute = true;
}

void NosXuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    room->playSkillEffect("xuanhuo");
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", objectName());
    room->obtainCard(effect.from, card_id, room->getCardPlace(card_id) != Player::Hand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, objectName());
    if(target != effect.from)
        room->obtainCard(target, card_id, false);
}

class NosXuanhuo: public OneCardViewAsSkill{
public:
    NosXuanhuo():OneCardViewAsSkill("nosxuanhuo"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("NosXuanhuoCard");
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return ! to_select->isEquipped() && to_select->getFilteredCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        NosXuanhuoCard *card = new NosXuanhuoCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class NosXuanfeng: public TriggerSkill{
public:
    NosXuanfeng():TriggerSkill("nosxuanfeng"){
        events << CardLost << CardLostDone;
    }

    virtual QString getDefaultChoice(ServerPlayer *) const{
        return "nothing";
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *lingtong, QVariant &data) const{
        if(event == CardLost){
            CardMoveStar move = data.value<CardMoveStar>();
            if(move->from_place == Player::Equip)
                lingtong->tag["InvokeNosXuanfeng"] = true;
        }else if(event == CardLostDone && lingtong->tag.value("InvokeNosXuanfeng", false).toBool()){
            lingtong->tag.remove("InvokeNosXuanfeng");

            QStringList choicelist;
            choicelist << "nothing";
            QList<ServerPlayer *> targets1;
            foreach(ServerPlayer *target, room->getAlivePlayers()){
                if(lingtong->canSlash(target, false))
                    targets1 << target;
            }
            if (!targets1.isEmpty()) choicelist << "slash";
            QList<ServerPlayer *> targets2;
            foreach(ServerPlayer *p, room->getOtherPlayers(lingtong)){
                if(lingtong->distanceTo(p) <= 1)
                    targets2 << p;
            }
            if (!targets2.isEmpty()) choicelist << "damage";

            QString choice;
            if (choicelist.length() == 1)
                return false;
            else
                choice = room->askForChoice(lingtong, objectName(), choicelist.join("+"));


            if(choice == "slash"){
                ServerPlayer *target = room->askForPlayerChosen(lingtong, targets1, "xuanfeng-slash");

                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("nosxuanfeng");

                CardUseStruct card_use;
                card_use.card = slash;
                card_use.from = lingtong;
                card_use.to << target;
                room->useCard(card_use, false);
            }else if(choice == "damage"){
                room->playSkillEffect("xuanfeng");

                ServerPlayer *target = room->askForPlayerChosen(lingtong, targets2, "xuanfeng-damage");

                DamageStruct damage;
                damage.from = lingtong;
                damage.to = target;
                room->damage(damage);
            }
        }

        return false;
    }
};

//gd1h
#include "maneuvering.h"

class NosJuejing: public TriggerSkill{
public:
    NosJuejing():TriggerSkill("nosjuejing"){
        events << CardLostDone << CardGotDone << PhaseChange;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *gaodayihao, QVariant &data) const{
        if(event == PhaseChange){
            if(gaodayihao->getPhase() == Player::Draw){
                room->playSkillEffect(objectName());
                return true;
            }
            return false;
        }
        if(gaodayihao->getHandcardNum() == 4)
            return false;
        int delta = gaodayihao->getHandcardNum() - 4;
        if(delta < 0){
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            gaodayihao->drawCards(qAbs(delta));
        }
        else{
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->askForDiscard(gaodayihao, objectName(), delta);
        }
        return false;
    }
};

class NosLonghun: public OneCardViewAsSkill{
public:
    NosLonghun():OneCardViewAsSkill("noslonghun"){
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash"
                || pattern == "jink"
                || pattern.contains("peach")
                || pattern == "nullification";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->isWounded() || Slash::IsAvailable(player);
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        switch(ClientInstance->getStatus()){
        case Client::Playing:{
                if(Self->isWounded() && card->getSuit() == Card::Heart)
                    return true;
                else if(Slash::IsAvailable(Self) && card->getSuit() == Card::Diamond)
                    return true;
                else
                    return false;
            }

        case Client::Responsing:{
                QString pattern = ClientInstance->getPattern();
                if(pattern == "jink")
                    return card->getSuit() == Card::Club;
                else if(pattern == "nullification")
                    return card->getSuit() == Card::Spade;
                else if(pattern == "peach" || pattern == "peach+analeptic")
                    return card->getSuit() == Card::Heart;
                else if(pattern == "slash")
                    return card->getSuit() == Card::Diamond;
            }

        default:
            break;
        }

        return false;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        Card *new_card = NULL;

        Card::Suit suit = card->getSuit();
        int number = card->getNumber();
        switch(card->getSuit()){
        case Card::Spade:{
                new_card = new Nullification(suit, number);
                break;
            }

        case Card::Heart:{
                new_card = new Peach(suit, number);
                break;
            }

        case Card::Club:{
                new_card = new Jink(suit, number);
                break;
            }

        case Card::Diamond:{
                new_card = new FireSlash(suit, number);
                break;
            }
        default:
            break;
        }

        if(new_card){
            new_card->setSkillName(objectName());
            new_card->addSubcard(card);
        }

        return new_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return static_cast<int>(card->getSuit()) + 1;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        foreach(const Card *card, player->getHandcards() + player->getEquips()){
            if(card->objectName() == "nullification")
                return true;

            if(card->getSuit() == Card::Spade)
                return true;
        }

        return false;
    }
};

class NosDuojian: public TriggerSkill{
public:
    NosDuojian():TriggerSkill("#noslonghun_duojian"){
        events << PhaseChange;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *gaodayihao, QVariant &data) const{
        if(gaodayihao->getPhase() == Player::Start){
            foreach(ServerPlayer *p, room->getOtherPlayers(gaodayihao)){
                if(p->hasWeapon("qinggang_sword") && room->askForSkillInvoke(gaodayihao, objectName())){
                    gaodayihao->obtainCard(p->getWeapon());
                    break;
                }
            }
        }
        return false;
    }
};

//yjcm2012
#include "god.h"
class NosGongqi : public OneCardViewAsSkill{
public:
    NosGongqi():OneCardViewAsSkill("nosgongqi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->getTypeId() == Card::Equip;
    }

    const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        WushenSlash *slash = new WushenSlash(card->getSuit(), card->getNumber());
        slash->addSubcard(card);
        slash->setSkillName(objectName());
        return slash;
    }
};

class NosJiefan : public TriggerSkill{
public:
    NosJiefan():TriggerSkill("nosjiefan"){
        events << Dying << SlashHit << SlashMissed << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *noshandang = room->findPlayerBySkillName(objectName());

        if(event == Dying){
            DyingStruct dying = data.value<DyingStruct>();
            if(!noshandang || !dying.savers.contains(noshandang) || dying.who->getHp() > 0 || noshandang->isNude() || !room->askForSkillInvoke(noshandang, objectName(), data))
                return false;

            const Card *slash = room->askForCard(noshandang, "slash", "nosjiefan-slash:" + dying.who->objectName(), data);
            slash->setFlags("nosjiefan-slash");
            room->setTag("NosJiefanTarget", data);
            if(slash){
                CardUseStruct use;
                use.card = slash;
                use.from = noshandang;
                use.to << room->getCurrent();
                room->useCard(use);
            }
        }
        else if(event == SlashHit){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(!player->hasSkill(objectName())
               || room->getTag("NosJiefanTarget").isNull())
                return false;

            DyingStruct dying = room->getTag("NosJiefanTarget").value<DyingStruct>();
            ServerPlayer *target = dying.who;
            room->removeTag("NosJiefanTarget");
            Peach *peach = new Peach(effect.slash->getSuit(), effect.slash->getNumber());
            peach->setSkillName(objectName());
            CardUseStruct use;
            use.card = peach;
            use.from = noshandang;
            use.to << target;
            room->useCard(use);

            return true;
        }
        else if(event == SlashMissed)
            room->removeTag("NosJiefanTarget");
        else
            if(!room->getTag("NosJiefanTarget").isNull())
                room->removeTag("NosJiefanTarget");

        return false;
    }
};

NostalGeneralPackage::NostalGeneralPackage()
    :Package("nostal_general")
{
    General *nosfazheng = new General(this, "nosfazheng", "shu", 3);
    nosfazheng->addSkill(new NosEnyuan);
    patterns.insert(".enyuan", new EnyuanPattern);
    nosfazheng->addSkill(new NosXuanhuo);

    General *noslingtong = new General(this, "noslingtong", "wu");
    noslingtong->addSkill(new NosXuanfeng);

    General *nosxushu = new General(this, "nosxushu", "shu", 3);
    nosxushu->addSkill(new NosWuyan);
    nosxushu->addSkill(new NosJujian);

    addMetaObject<NosXuanhuoCard>();
    addMetaObject<NosJujianCard>();

    General *noshandang = new General(this, "noshandang", "wu");
    noshandang->addSkill(new NosGongqi);
    noshandang->addSkill(new NosJiefan);

    General *sp_shenzhaoyun = new General(this, "sp_shenzhaoyun", "god", 1, true, true);
    sp_shenzhaoyun->addSkill(new NosJuejing);
    sp_shenzhaoyun->addSkill(new NosLonghun);
    sp_shenzhaoyun->addSkill(new NosDuojian);
    related_skills.insertMulti("noslonghun", "#noslonghun_duojian");
}

ADD_PACKAGE(Nostalgia)
ADD_PACKAGE(NostalGeneral)

