# -*- coding: utf-8 -*-

import  argparse, dataset_walker, json, time, copy, math, operator
from collections import namedtuple
from argparse import RawTextHelpFormatter
import sys

Ontology = {}

def load_ontology(file) : 
    json_data = open(file)
    return json.load(json_data)


def utterance_to_str(utt):
    return ",".join([actsv['act'] + "(" + (','.join([("=".join(map(str,sv))) for sv in actsv['slots']]))+")" for actsv in utt])

def print_utterance(utt):
    print(utterance_to_str(utt))

def slu_hyps_to_str(slu_hyps):
    return "\n".join([utterance_to_str(hyp['slu-hyp']) + ': ' + str(hyp['score']) for hyp in slu_hyps])

def print_slu(slu_hyps):
    print("SLU hypotheses : \n" + slu_hyps_to_str(slu_hyps))

'''
YARBUS represents the goals as a dictionnary mapping joint goals to 
their probability 
'''
class YARBUS_Tracker(object):
    '''
    informable is a list of informable slots
    '''
    def __init__(self, informable, thr_belief, rule_set_number):
        self.informable = informable
        print("I parsed the following informable slots : " + str(informable))
        print("Yarbus only extracts the slots from the ontology actually (and not the possible values)")
        self.thr_belief = thr_belief
        print("I'm using the rule set number " + str(rule_set_number))
        self.compute_rule_set(rule_set_number)
        print("It corresponds to the use of the following rule : ")
        for rname, use_rule in self.rule_set.iteritems():
            print("%s rule : %i " % (rname, use_rule))
        self.reset()

    def compute_rule_set(self, rule_set_number):
        self.rule_set = {}
        rule_names = ["deny", "negate", "implconf", "explconf", "inform"]
        for n in rule_names:
            self.rule_set[n] = (rule_set_number % 2)
            rule_set_number /= 2

    def renormalize_slu(self, slu_hyps):
        score = 0.0
        for h in slu_hyps:
            score += h['score']
        for h in slu_hyps:
            h['score'] /= score
        return score != 1

    def rewrite_repeat(self, macts, prev_macts):
        for act in macts:
            if(act["act"] == "repeat"):
                if(len(macts) != 1):
                    print("I'm not expecting several acts when there is a repeat in the machine utterance")
                    return macts
                else:
                    return prev_macts
        return macts
        
    
    def extract_this_replacement(self, macts):
        replacement_set = set()
        for act in macts:
            if(act["act"] in ["expl-conf", "select"]):
                for sv in act["slots"]:
                    replacement_set.add(sv[0])
            elif(act["act"] == "request"):
                for sv in act["slots"]:
                    replacement_set.add(sv[1])
        return replacement_set
        
    def has_this(self, uact):
        has_this = False
        for sv in uact["slots"]:
            has_this = (sv[0] == "this")
            if(has_this):
                break
        return has_this

    def solve_this_reference_act(self, uact, slots_from_mact):
        if(self.has_this(uact)):
            if(len(slots_from_mact) == 1):
                # The reference can be solved
                for sv in uact["slots"]:
                    if(sv[0] == "this"):
                        sv[0] = next(iter(slots_from_mact))
                return uact
            else:
                # The reference cannot be solved,
                # we remove the this from the slots
                new_act = {"act": uact["act"], "slots": []}
                for sv in uact["slots"]:
                    if((sv[0] != "this")):
                        new_act["slots"].append(sv)
                # In case we don't have any remaining slot/value pairs
                # the act is dropped
                if(len(new_act["slots"]) == 0):
                    return None
                else:
                    return new_act
        else:
            return uact
        
    def solve_this_reference_utterance(self, uacts, slots_from_mact):
        new_uacts = []
        for uact in uacts:
            new_uact = self.solve_this_reference_act(uact, slots_from_mact)
            if(new_uact): # we got an act from solving the reference
                try:
                    index = new_uacts.index(new_uact)
                except ValueError:
                    # new_uact is not yet in the list, we add it
                    new_uacts.append(new_uact)
        return new_uacts

    def solve_this_reference(self, slu_hyps, macts):
        replacement_set = self.extract_this_replacement(macts)
        new_slu = []
        raw_hyps = []

        # Iterate over the hypotheses
        for slu_hyp_i in slu_hyps:
            utterance = slu_hyp_i['slu-hyp']
            score = slu_hyp_i['score']
            new_utterance = self.solve_this_reference_utterance(utterance, replacement_set)
            # We check if the utterance is already in the list
            try:
                index = raw_hyps.index(new_utterance)
                #index = new_slu.index(new_utterance)
                # In case this does not throw an exception, the utterance
                # is already in the list
                new_slu[index]['score'] += score
            except ValueError:
                new_slu.append({'slu-hyp': new_utterance, 'score':score})
                raw_hyps.append(new_utterance)
        return new_slu

    def get_posmus(self, machine_utt, user_utt, slot):
        pos = set()
        # Rule 1
        if(self.rule_set["inform"]):
            for utt in user_utt:
                if(utt['act'] == "inform"):
                    for s,v in utt['slots']:
                        if(slot == s):
                            pos.add(v)
        # Rule 2
        if(self.rule_set["explconf"]):
            has_affirm = False
            for utt in user_utt:
                has_affirm = (utt['act'] == "affirm")
                if(has_affirm):
                    break
            if(has_affirm):
                for utt in machine_utt:
                    if(utt['act'] == "expl-conf"):
                        for s,v in utt['slots']:
                            if(slot == s):
                                pos.add(v)
        # Rule 3
        if(self.rule_set["implconf"]):
            has_negate = False
            for utt in user_utt:
                has_negate = (utt['act'] == "negate")
                if(has_negate):
                    break
            if((not has_negate) and (len(user_utt) != 0)):
                for utt in machine_utt:
                    if(utt['act'] == "impl-conf"):
                        for s,v in utt['slots']:
                            if(slot == s):
                                pos.add(v)   
        return pos

    def get_negmus(self, machine_utt, user_utt, slot):
        neg = set()

        # Rule 4 : deny rule
        if(self.rule_set["deny"]):
            for utt in user_utt:
                if(utt['act'] == "deny"):
                    for s,v in utt['slots']:
                        if(slot == s):
                            neg.add("!" + v)

        # Rule 5 : negate rule
        if(self.rule_set["negate"]):
            has_negate = False
            for utt in user_utt:
                has_negate = (utt['act'] == "negate")
                if(has_negate):
                    break
            if(has_negate):
                for utt in machine_utt:
                    if(utt['act'] == "expl-conf"):
                        for s,v in utt['slots']:
                            if(slot == s):
                                neg.add("!" + v)   

        return neg

    '''
    Merge the positive and negatives
    pos_mus and neg_mus are sets
    Returns : a list of lists
    e.g. pos = {'north', 'south'}
         neg = {'!north', '!east', '!south'}
    Returns : [ ['north'],  ['south'] , ['!north', '!south'] ]
    '''
    def merge_mus(self, pos_mus, neg_mus):
        inf_mus = []
        if(len(pos_mus) == 0 and len(neg_mus) == 0):
            # In this case, we don't know anything
            inf_mus.append(["?"])
        elif len(pos_mus) == 0:
            # No positives so we keep all the negatives (there is at least 1)
            inf_mus.append(list(neg_mus))
        else:
            # Add the positives as singletons
            for p in pos_mus:
                inf_mus.append([p])
            # And all the negatives in conflict
            inf_neg = []
            for n in neg_mus:
                # n is necessarily a string like "!xxx"
                if n[1:] in pos_mus:
                    inf_neg.append(n)
            if(len(inf_neg) != 0):
                inf_mus.append(inf_neg)
        return inf_mus

    '''
    Given a dictionnary : (string, string set list) dic
    returns a  :  ((string, string set) dic)  list
    '''
    def expand_info(self,map_info):
        l = []
        for k, vset in map_info.iteritems():
            l1 = []
            for vset_i in vset:
                if(len(l) == 0):
                    l1.append({k: vset_i})
                else:
                    for di in l:
                        di_cpy = copy.deepcopy(di)
                        di_cpy[k] = vset_i
                        l1.append(di_cpy)
            l = l1
        return l

    def extract_info_from_utterance(self, macts, uacts):
        # Map_info is a dictionnary with keys as informable slots
        # and values as a list of a set of values
        # e.g
        # { area: [{north},{south},{!south,!north}], price: [{?}] }
        # Returns a list of dictionnaries:
        # [{area: {north}, price:{?}}, {area:{south}, price:{?}}, {area:{!north,!south}, price:{?}}]
        map_info = {}
        for s in self.informable:
            pos_mus = self.get_posmus(macts, uacts, s)
            neg_mus = self.get_negmus(macts, uacts, s)
            inf_mus = self.merge_mus(pos_mus, neg_mus)
            map_info[s] = inf_mus
        return self.expand_info(map_info)

    def extract_info(self, macts, slu_hyps):
        # Information extraction
        # weighted_info is a list of probabilised informations
        # e.g. [({area: {?}, price: {cheap} }, 0.9), ({area:{?}, price:{?}}, 0.1)]
        weighted_info = []
        for hyp in slu_hyps:
            uacts = hyp['slu-hyp']
            score = hyp['score']
            infos = self.extract_info_from_utterance(macts, uacts)
            single_score = score / float(len(infos))
            weighted_info += [(i, single_score) for i in infos]
        return weighted_info


    '''
    Input : a list of tuples (dictionnary, score)
    that we simplify. These elements come from the multiple hypotheses that we merge
    '''
    def merge_infos(self, info):
        merged_info = []
        raw_infos = []
        for inf, score in info:
            try:
                index = raw_infos.index(inf)
                # The element is already in the raw_infos, we add the score
                merged_info[index] = (inf, merged_info[index][1]+score)
            except ValueError:
                merged_info.append((inf, score))
                raw_infos.append(inf)
        return merged_info

    def is_negative(self, info):
        return info[0] == "!"

    def is_positive(self, info):
        return not self.is_negative(info)

    def is_unknown(self, info):
        return info == "?"

    def is_set_negative(self, info_set):
        if(len(info_set) == 1):
            return self.is_negative(next(iter(info_set)))
        else:
            if(not all(map(self.is_negative, info_set))):
                raise RuntimeError("If the info has more than one element these are expected to be all negatives...")
            return True


    def transition_function_slot(self, goal_slot, info_slot_set):
        if((len(info_slot_set) == 1) and self.is_unknown(next(iter(info_slot_set)))):
            return goal_slot
        elif ((self.is_unknown(goal_slot)) and (len(info_slot_set) == 1) and (self.is_positive(next(iter(info_slot_set))))):
            return next(iter(info_slot_set))
        elif ((self.is_unknown(goal_slot)) and self.is_set_negative(info_slot_set)):
            return "?"
        else:
            # In this case, we necessarily have goal_slot in val_*
            v = goal_slot
            if(v == "?"):
                raise RuntimeError("In this case, the goal should not be \"?\"")
            if( (len(info_slot_set) == 1) and (self.is_positive(next(iter(info_slot_set))))):
                return next(iter(info_slot_set))
            elif ("!"+v) in info_slot_set:
                return "?"
            else:
                return v
    '''
    Given a goal (string,string) dic 
      and an info (string, string set) dic
    Returns the new goal
    e.g.  goal = {area : ?, price : cheap}
          info = {area : {north}, price : {!cheap, !expensive}}
    returns  goal' = {area: north, price : ?}
    '''
    def transition_function(self, goal, info):
        new_goal = {}
        for s, goal_slot in goal.iteritems():
            new_goal[s] = self.transition_function_slot(goal_slot, info[s])
        return new_goal

    def update_goals(self, infos):
        new_belief = []
        raw_goals = []
        for goal, score_goal in self.belief:
            for info, score_info in infos:
                new_goal = self.transition_function(goal, info)
                try:
                    index_goal = raw_goals.index(new_goal)
                    # In this case, the goal is already in the belief at position index_goal
                    new_belief[index_goal] = (new_goal, new_belief[index_goal][1] + score_goal * score_info)
                except ValueError:
                    new_belief.append((new_goal, score_goal * score_info))
                    raw_goals.append(new_goal)
        self.belief = new_belief

    def clean_belief(self, thr):
        all_below_thr = all([score < thr for g, score in self.belief])
        if(not all_below_thr):
            new_belief = []
            sum_scores = 0.0
            for g, score in self.belief:
                if(score >= thr):
                    sum_scores += score
                    new_belief.append((g, score))
            self.belief = [(g, score/sum_scores) for g,score in new_belief]

    def update_methods(self, macts, slu_hyps):
        transition = {"byalternatives" : 0.0, "byconstraints" : 0.0, "byname" : 0.0, "finished" : 0.0, "none":0.0}
        for hyp in slu_hyps:
            uacts = hyp['slu-hyp']
            score = hyp['score']
            has_reqalts = False
            has_inform_not_name = False
            has_inform_name = False
            has_bye = False
            for utt in uacts:
                if(utt['act'] == 'reqalts') : has_reqalts = True
                elif(utt['act'] == 'inform') : 
                    for s,v in utt['slots']:
                        if(s != 'name'): 
                            has_inform_not_name = True
                        else :
                            has_inform_name = True
                elif(utt['act'] == 'bye'):
                    has_bye = True
            n = int(has_reqalts) + int(has_inform_not_name) + int(has_inform_name) + int(has_bye)
            # The mass of the hyp is split uniformely over the candidates
            if(has_reqalts or has_inform_not_name or has_inform_name or has_bye):
                transition["byalternatives"] += score * float(has_reqalts) / float(n)
                transition["byconstraints"] += score * float(has_inform_not_name) / float(n)
                transition["byname"] += score * float(has_inform_name) / float(n)
                transition["finished"] += score * float(has_bye) / float(n)
            else:
                transition["none"] += score
        # We can now update the methods recursively
        for l in self.method_label.iterkeys():
            if( l != "none"):
                self.method_label[l] = transition["none"] * self.method_label[l] + transition[l]
            else:
                self.method_label[l] = transition["none"] * self.method_label[l]

    def update_requested_slots(self, macts, slu_hyps):
        # We first removed the informed slots
        for utt in macts:
            if(utt['act'] == "inform"):
                for s,v in utt['slots']:
                    if s in self.requested_slots:
                        self.requested_slots.pop(s, None)

        # We then marginalize the request(slot=s) acts from the user
        request_scores = {}
        for hyp in slu_hyps:
            uacts = hyp['slu-hyp']
            score = hyp['score']
            slots_for_this_hyp = []
            for utt in uacts:
                if(utt['act'] == 'request'):
                    for shctruk, s in utt['slots']:
                        if not s in slots_for_this_hyp:
                            if s in request_scores:
                                request_scores[s] += score
                            else:
                                request_scores[s] = score
                            slots_for_this_hyp.append(s)

        # The probability P_t(s) of requesting the slot s can be computed recursively as
        # P_t(s) = P_{t-1}(s) + (1 - P_{t-1}) P(request_t(s))
        for s,score in request_scores.items():
            if s in self.requested_slots:
                self.requested_slots[s] += (1-self.requested_slots[s]) * score
            else:
                self.requested_slots[s] = score

    def addTurn(self, turn, verbose=False):

        slu_hyps = turn['input']['live']['slu-hyps']
        macts = turn["output"]["dialog-acts"]

        if(verbose):
            print("Original machine act : " + utterance_to_str(macts))
            print("Original SLU : \n" + slu_hyps_to_str(slu_hyps))
            print("\n")
        
        # Preprocessing the SLU
        self.renormalize_slu(slu_hyps)
        macts = self.rewrite_repeat(macts, self.prev_mact)
        slu_hyps = self.solve_this_reference(slu_hyps, macts)

        if(verbose):
            print("Rewritten machine act : " + utterance_to_str(macts))
            print("Rewritten SLU : \n" + slu_hyps_to_str(slu_hyps))
            print("\n")

        # infos is a list of probabilised informations
        # e.g. [({area: {?}, price: {cheap} }, 0.9), ({area:{?}, price:{?}}, 0.1)]
        infos = self.extract_info(macts, slu_hyps)
        infos = self.merge_infos(infos)
        if(verbose):
            print("Extracted informations : ")
            for i in infos:
                print(i)

        self.update_goals(infos)
        self.clean_belief(self.thr_belief)

        self.update_methods(macts, slu_hyps)
        self.update_requested_slots(macts, slu_hyps)

        if(verbose):
            self.print_me()

        self.prev_mact = macts


        goal_labels_joint = []
        for g, score in self.belief:
            is_full_unknown = all([v == "?" for k,v in g.iteritems()])
            if(not is_full_unknown):
                goal_labels_joint.append({"slots": {k:v for k,v in g.iteritems() if v != "?"}, "score": score})
        # goal-labels-joint : joint_hyps = [{"slots":{}, "score" : 1.0}]

        return {"goal-labels":{}, "goal-labels-joint":goal_labels_joint, "requested-slots": copy.copy(self.requested_slots), "method-label": copy.copy(self.method_label)}

    def reset(self):
        self.prev_mact = []

        self.requested_slots = {}
        self.method_label = {"byconstraints" : 0.0, "byname" : 0.0, "byalternatives" : 0.0, "finished" : 0.0, "none" : 1.0}

        self.belief = []
        unknown_goal = {}
        for s in self.informable:
            unknown_goal[s] = "?"
        self.belief.append( (unknown_goal, 1.0))

    def print_me(self):
        print("Belief : \n")
        for g, score in self.belief:
            print("(" + ",".join([s + "=" + v for s,v in g.iteritems()]) + ") : " + str(score))
        print("Methods : " + str(self.method_label))
        print("Requested slots : " + str(self.requested_slots))


def print_gplv3() :
    print '\nThis program is free software: you can redistribute it and/or modify\n'+\
          'it under the terms of the GNU General Public License as published by\n'+\
          'the Free Software Foundation, either version 3 of the License, or\n'+\
          '(at your option) any later version.\n\n'+\
          'This program is distributed in the hope that it will be useful,\n'+\
          'but WITHOUT ANY WARRANTY; without even the implied warranty of\n'+\
          'MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n'+\
          'GNU General Public License for more details.\n\n'+\
          'You should have received a copy of the GNU General Public License\n'+\
          'along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n'

def main() :
    
    #print_gplv3()

    parser = argparse.ArgumentParser(description='YARBUS Rule-based Dialog State Tracker Baseline V1.0\n  by Jeremy Fix\t jeremy.fix@centralesupelec.fr \n  and Hervé Frezza-Buet\t herve.frezza-buet@centralesupelec.fr\n',\
                                     formatter_class=RawTextHelpFormatter)
    parser.add_argument('--dataset', dest='dataset', action='store', metavar='DATASET', required=True,
                        help='The dataset to analyze')
    parser.add_argument('--ontology', dest='ontology', action='store', metavar='JSON_FILE', required=True,
                        help='The ontology to use')
    parser.add_argument('--dataroot',dest='dataroot',action='store',required=True,metavar='PATH',
                        help='Will look for corpus in <destroot>/<dataset>/...')
    parser.add_argument('--trackfile',dest='trackfile',action='store',required=True,metavar='JSON_FILE',
                        help='File to write with tracker output')
    parser.add_argument('--thr_belief', dest='thr_belief', action='store', required=False, default=0.0,type=float,
                        help='Sets the threshold below which the hypothesis in the belief are removed')
    parser.add_argument('--rule_set', dest='rule_set', action='store', required=False, default=31, type=int,
                        help='Specifies which rule set to use, an int in [0, 31]')
    parser.add_argument('--session_id',dest='session_id',action='store',required=False,metavar='voip-...',
                        help='A particular session id to run on')

    args = parser.parse_args()

    
    dataset = dataset_walker.dataset_walker(args.dataset, dataroot=args.dataroot)
    session_id = None
    if args.session_id:
        session_id = args.session_id
        print 'Running on session_id ' + session_id
    verbose = (session_id != None)

    ontology = load_ontology(args.ontology)

    track_file = open(args.trackfile, "wb")
    track = {"sessions":[]}
    track["dataset"]  = args.dataset
    start_time = time.time()
    print("Yarbus will prune its belief with a threshold of %f ; to change this, check out the option --thr_belief" % args.thr_belief)
    tracker = YARBUS_Tracker(ontology["informable"].keys(), args.thr_belief, args.rule_set)

    nb_dialogs = len(dataset)
    print("%i dialogs to process" % nb_dialogs)
    for index_call, call in enumerate(dataset):
        if(session_id and session_id != call.log["session-id"]):
            continue
        if(verbose):
            print("Processing session : " + call.log["session-id"])
        else:
            sys.stdout.write('\r Processing dialog %i / %i' % (index_call+1, nb_dialogs))
            sys.stdout.flush()

        this_session = {"session-id":call.log["session-id"], "turns":[]}
        tracker.reset()
        for index_turn, (turn, _) in enumerate(call):
            if(verbose):
                print("*" * 10 + " Turn " + str(index_turn+1) + " " + "*"*10)
            tracker_turn = tracker.addTurn(turn, verbose)
            this_session["turns"].append(tracker_turn)
        
        track["sessions"].append(this_session)
    sys.stdout.write('\n')
    end_time = time.time()
    elapsed_time = end_time - start_time
    track["wall-time"] = elapsed_time

    json.dump(track, track_file,indent=4)

if __name__ == '__main__':
    main()
    
