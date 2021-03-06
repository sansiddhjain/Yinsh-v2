//
// Created by karthik on 13/9/18.
//

#include <string>
#include <cmath>
#include <algorithm>
#include "Agent.h"
#include <iostream>
#include <cstdlib>
#include <cfloat>
#include <fstream>

#define STALEMATE_SCORE 5
#define NUM_FEATURES 22

using namespace std;

void Agent::execute_move(string move, int playerID) {
    state.execute_move(move, playerID);
}

// Recursively construct tree normally
void Agent::recursive_construct_tree(Board board, Node *node, int depth, int maxDepth) {
    if (depth < maxDepth) //Can tune this 4 parameter
    {
        node->isLeaf = false;
        vector<pair<pair<int, int>, pair<int, int> > > succ_all;
        if (depth % 2 == 0) //Self player is playing
        {
            node->type = 'M';
            for (int i = 0; i < state.num_rings_on_board; i++) {
                pair<pair<int,int>,pair<int,int>> temp = make_pair(make_pair(-1,-1), make_pair(-1,0));
                vector<pair<pair<int, int>, pair<int, int> > > succ_ring = board.successors(board.rings_vector.at(i));
                succ_all.reserve(succ_all.size() + succ_ring.size());
                succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
            }
        }
        else //Opponent is playing
        {
            node->type = 'm';
            for (int i = 0; i < state.num_opp_rings_on_board; i++) {
                vector<pair<pair<int, int>, pair<int, int> > > succ_ring = board.successors(board.opp_rings_vector.at(i));
                succ_all.reserve(succ_all.size() + succ_ring.size());
                succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
            }
        }
        node->children = new Node *[succ_all.size()];
        vector<pair<pair<int, int>, pair<int, int> > >::iterator ptr;
        int idx = 0;
        for (ptr = succ_all.begin(); ptr != succ_all.end(); ptr++) {
            pair<pair<int, int>, pair<int, int> > move = *ptr;
            node->children[idx] = new Node;
            node->children[idx]->move = move;
            Board b(board);
            bool c = b.move_ring(move.first, move.second);
            if (c) {
                recursive_construct_tree(b, node->children[idx], depth + 1, maxDepth);
            }
            idx++;
        }
    }
    else {
        node->isLeaf = true;
        node->score = board.calculate_score();
    }
}

string Agent::initial_move() {
    //todo: change strategy, also executes_move
    // for now just place near centre, first move on centremost hexagons going clockwise
    string move;
    if (!state.game_board.at(0).at(0).is_piece()) {
        state.place_piece('r', state.player_color, pair<int, int>(0, 0));
        return "P 0 0 ";
    }
    for (int j = 0; j < n; j++) {
        pair<int, int> pos_xy = state.hex_to_xy(pair<int, int>(1, j));
        if (!state.game_board.at(pos_xy.first).at(pos_xy.second).is_piece()) {
            state.place_piece('r', state.player_color, pair<int, int>(pos_xy.first, pos_xy.second));
            return "P 1 " + to_string(j);
        }
    }
    for (int j = 0; j < 2 * n; j++) {
        pair<int, int> pos_xy = state.hex_to_xy(pair<int, int>(2, j));
        if (!state.game_board.at(pos_xy.first).at(pos_xy.second).is_piece()) {
            state.place_piece('r', state.player_color, pair<int, int>(pos_xy.first, pos_xy.second));
            return "P 2 " + to_string(j);
        }
    }
}

string Agent::get_next_move() {
    // IMPORTANT - Also executes next move
    if (state.num_moves_played < 10) {
        // Perform placement of ring
        cerr << "In initial move" << endl;
        state.num_moves_played++;
        return initial_move();
    }
    state_tree = Board(state);
    Node *tree = new Node;
    tree->type = 'M';
    // 3 3 4 is doing good
    if (state.num_moves_played < 16) {
        minimax_ab_sort(state_tree, tree, 0, -INFINITY, INFINITY, 5);
    }
    else if (state.num_moves_played < 22) {
        minimax_ab_sort(state_tree, tree, 0, -INFINITY, INFINITY, 5);
    }
    else {
        minimax_ab_sort(state_tree, tree, 0, -INFINITY, INFINITY, 5);
    }

    pair<pair<int, int>, pair<int, int> > move = tree->children[tree->gotoidx]->move;
    std::cerr << move.first.first << ", " << move.first.second << "; " << move.second.first << ", "
              << move.second.second << '\n';

    // convert to string to send to server
    string output = state.execute_move_and_return_server_string(move);
    return output;
}

// Vanilla minimax
double Agent::minimax(Node *node) {
    if (node->isLeaf) { return node->score; }
    if (node->type == 'M') // MAX node
    {
        double max_score = 0;
        int max_idx = 0;
        size_t n = sizeof(node->children) / sizeof(node->children[0]);
        for (int i = 0; i < n; i++) {
            if (minimax(node->children[i]) > max_score) {
                max_score = node->children[i]->score;
                max_idx = i;
            }
        }
        node->score = max_score;
        node->gotoidx = max_idx;
        return max_score;
    }
    else // MIN node
    {
        double min_score = node->children[0]->score;
        int min_idx = 0;
        size_t n = sizeof(node->children) / sizeof(node->children[0]);
        for (int i = 0; i < n; i++) {
            if (minimax(node->children[i]) < min_score) {
                min_score = node->children[i]->score;
                min_idx = i;
            }
        }
        node->score = min_score;
        node->gotoidx = min_idx;
        return min_score;
    }
}


NodeMCTS* Agent::select(NodeMCTS* root){
    if(root->isLeaf)
    {
        std::cerr << "end selection" << '\n';
        return root;
    }
    else {
        //find best child
        double maxUCB1 = -DBL_MAX;
        NodeMCTS* bestChild = nullptr;
        int i = 0, bestChildIdx = 0;
        for(auto& child : root->children) {
            if (child->isLeaf) {
              bestChildIdx = i;
              bestChild = child;
              break;
            }
            double ucb = child->ucb();
            // std::cerr << ucb << ' ';
            if(ucb > maxUCB1) {
                maxUCB1 = ucb;
                bestChild = child;
                bestChildIdx = i;
            }
            i++;
        }
        // std::cerr << '\n';
        std::cerr << bestChildIdx << ' ';
        return select(bestChild);
    }
}

// randomly chooses one of the successors and adds to tree
void Agent::expand(NodeMCTS* node) {
    node->isLeaf = false;
    vector<pair<pair<int, int>, pair<int, int> > > succ_all;
    if (node->depth % 2 == 0) {//Self player is playing
        node->type = 'M';
        for (int i = 0; i < state.num_rings_on_board; i++) {
            vector<pair<pair<int, int>, pair<int, int> > > succ_ring = node->configuration->successors(node->configuration->rings_vector.at(i));
            succ_all.reserve(succ_all.size() + succ_ring.size());
            succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
        }
    }
    else {//Opponent is playing
        node->type = 'm';
        for (int i = 0; i < state.num_opp_rings_on_board; i++) {
            vector<pair<pair<int, int>, pair<int, int> > > succ_ring = node->configuration->successors(node->configuration->opp_rings_vector.at(i));
            succ_all.reserve(succ_all.size() + succ_ring.size());
            succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
        }
    }

    std::cerr << succ_all.size() << '\n';
    if (succ_all.empty()) {
      node->isLeaf = true;
      return;
    }

    // Board temp_board;
    for (int i = 0; i < succ_all.size(); i++)
    {
      NodeMCTS* child = new NodeMCTS(node);
      child->depth++;
      child->move = succ_all.at(i);
      child->parent = node;
      child->type = node->type == 'M' ? 'm' : 'M';
      Board *temp_board = new Board(*node->configuration);
      temp_board->execute_move_and_return_server_string(child->move);
      child->configuration = temp_board;
      node->children.emplace_back(child);
    }

    // srand(time(NULL));
    // int i = rand() % succ_all.size();
    // node->children.at(i)->isLeaf = false;
    // return node->children.at(i);
    return;
}

//plays random move till finish and returns score
int Agent::simulate(NodeMCTS* node) {
    NodeMCTS simulator(*node);
    simulator.configuration = node->configuration;
    // std::cerr << simulator.depth << '\n';
    // std::cerr << simulator.type << '\n';
    // std::cerr << "entered simulate" << '\n';
    // std::cerr << check_end(*simulator.configuration) << '\n';
    int no_of_steps = 0;
    while(check_end(*simulator.configuration) < 0) {
      // std::cerr << "entered while loop" << '\n';
        vector<pair<pair<int, int>, pair<int, int> > > succ_all;
        if (simulator.depth % 2 == 0) {//Self player is playing
            // std::cerr << "Max node" << '\n';
            simulator.type = 'M';
            for (int i = 0; i < simulator.configuration->num_rings_on_board; i++) {
                vector<pair<pair<int, int>, pair<int, int> > > succ_ring = simulator.configuration->successors(simulator.configuration->rings_vector.at(i));
                // std::cerr << "calculated successors of one ring" << '\n';
                succ_all.reserve(succ_all.size() + succ_ring.size());
                succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
                // std::cerr << "appended to succ_all" << '\n';
            }
        }
        else {//Opponent is playing
            simulator.type = 'm';
            for (int i = 0; i < simulator.configuration->num_opp_rings_on_board; i++) {
                vector<pair<pair<int, int>, pair<int, int> > > succ_ring = simulator.configuration->successors(simulator.configuration->opp_rings_vector.at(i));
                succ_all.reserve(succ_all.size() + succ_ring.size());
                succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
            }
        }

        if(succ_all.empty())
            return STALEMATE_SCORE + simulator.configuration->num_opp_rings_on_board - simulator.configuration->num_rings_on_board;

        // srand(time(NULL));
        int i = rand() % succ_all.size();
        simulator.type = simulator.type == 'M' ? 'm' : 'M';
        simulator.depth++;
        simulator.configuration->execute_move_and_return_server_string(succ_all.at(i));
        no_of_steps++;
    }
    std::cerr << "Simulated till " << no_of_steps << " steps" << '\n';
    return check_end(*simulator.configuration);
}

void Agent::backpropagate(NodeMCTS* node, int score) {
    if(node == nullptr)
        return;
    if(node->type == 'M')
        node->totalValue += 10 - score;
    else
        node->totalValue += score;
    node->visits++;
    backpropagate(node->parent, score);
}


pair<pair<int,int>, pair<int,int>> Agent::monte_carlo(int iterations) {

    NodeMCTS* root = new NodeMCTS(nullptr);
    root->configuration = new Board(this->state);

    // for(int i = 0; i < iterations; i++){
    for(int i = 0; i < 35; i++){
        NodeMCTS* selected = select(root);
        std::cerr << '\n';
        expand(selected);
        std::cerr << "root children size - " << root->children.size() << '\n';
        std::cerr << "Whether root is leaf - " << root->isLeaf << '\n';
        std::cerr << "Whether selected is leaf - " << selected->isLeaf << '\n';
        // std::cerr << "expanded" << '\n';
        int score = simulate(selected);
        std::cerr << "score after simulation - " << score << '\n';
        backpropagate(selected, score);
        std::cerr << "selected total value " << selected->totalValue << '\n';
        std::cerr << "selected total no of visits " << selected->visits <<  '\n';
        std::cerr << "root total value " << root->totalValue << '\n';
        std::cerr << "root total no of visits " << root->visits <<  '\n';
        std::cerr << "Iteration " << i << " done" << '\n';
        int j = 0;
        for(auto& child : root->children) {
            if(!child->isLeaf) {
                std::cerr << "For idx " << j << " total value  - " << child->totalValue << " total visits  - " << child->visits << '\n';
            }
            j++;
        }
    }

    double bestScore = -DBL_MAX;
    NodeMCTS* bestChild = nullptr;
    for(auto& child : root->children) {
        if(child->totalValue/child->visits > bestScore) {
            bestChild = child;
            bestScore = child->totalValue/child->visits;
        }
    }

    delete root;
    return bestChild->move;
}

double Agent::minimax_ab_sort(Board board, Node *node, int depth, double alpha, double beta, int maxDepth) {

    if (!board.get_marker_rows(board.return_k(), board.player_color).empty()) {
        return board.calculate_score();
    }

    if (!board.get_marker_rows(board.return_k(), board.other_color).empty()) {
        return board.calculate_score();
    }

    if (depth == maxDepth) {
        // increase depth if we have 4 in a row here
        return board.calculate_score();
    }
    else {
        node->isLeaf = false;
        multimap< double, pair<pair<int, int>, pair<int, int> > > succ_all;
        if (depth % 2 == 0) {//Self player is playing
            node->type = 'M';
            for (int i = 0; i < state.num_rings_on_board; i++) {
                vector< pair< double, pair<pair<int, int>, pair<int, int> > > > succ_ring = board.successors_score(board.rings_vector.at(i));
                succ_all.insert(succ_ring.begin(), succ_ring.end());
            }
        }
        else {//Opponent is playing
            node->type = 'm';
            for (int i = 0; i < state.num_opp_rings_on_board; i++) {
                vector< pair< double, pair<pair<int, int>, pair<int, int> > > > succ_ring = board.successors_score(board.opp_rings_vector.at(i));
                succ_all.insert(succ_ring.begin(), succ_ring.end());
            }
        }

        node->children = new Node*[succ_all.size()];

        //increase if small branching factor at this node
//        cerr << "Branching factor = " << succ_all.size() << endl;

        multimap< double, pair<pair<int, int>, pair<int, int> > >::iterator it;
        if(node->type == 'M') {
            double v = -INFINITY;
            int i = 0;
            for (it=--succ_all.end(); it!=succ_all.begin(); --it){
                node->children[i] = new Node;
                node->children[i]->move = it->second;
                node->children[i]->type = 'm';
                Board temp_board(board);
                temp_board.move_ring(it->second.first, it->second.second);
                double v_prime = minimax_ab_sort(temp_board, node->children[i], depth + 1, v, beta, maxDepth);
                if(v_prime > v){
                    v = v_prime;
                    node->gotoidx = i;
                }
                if((int) v_prime == 10000){
                    cerr << "BROKEN" << endl;
                    break;
                }
                alpha = max(alpha, v);
                if (alpha >= beta)
                    break;
                i++;
            }
            return v;
        }
        else {
            double v = INFINITY;
            int i = 0;
            for (it=succ_all.begin(); it!=succ_all.end(); ++it){
                node->children[i] = new Node;
                node->children[i]->move = it->second;
                node->children[i]->type = 'M';
                Board temp_board(board);
                temp_board.move_ring(it->second.first, it->second.second);
                double v_prime = minimax_ab_sort(temp_board, node->children[i], depth + 1, alpha, v, maxDepth);
                if(v_prime < v) {
                    v = v_prime;
                    node->gotoidx = i;
                }
                if((int) v_prime == -10000.0){
                    break;
                }
                beta = min(beta, v);
                if (alpha >= beta)
                    break;
                i++;
            }
            return v;
        }
    }
}

int Agent::check_end(Board state) {
    if (state.num_rings_on_board == (state.return_m() - state.return_l())){
        return 10 - (state.return_m() - state.num_opp_rings_on_board);
    }
    else if (state.num_opp_rings_on_board == (state.return_m() - state.return_l())) {
        return (state.return_m() - state.num_rings_on_board);
    }
    else
        return -1;
}

void Agent::initialize_weights(){
    feature_weights = vector<double>(NUM_FEATURES, 0);
    for(int i = 0; i < feature_weights.size(); i++)
        feature_weights.at(i) = ((double) rand())/RAND_MAX;
}

//returns max Q value of the state for all actions possible (returns V value) and corresponding move
pair<double, pair<pair<int,int>,pair<int,int>>> Agent::get_best_Qmove(Board& state) {
    vector<pair<pair<int, int>, pair<int, int> > > succ_all;
    for (int i = 0; i < state.num_rings_on_board; i++) {
        vector<pair<pair<int, int>, pair<int, int> > > succ_ring = state.successors(state.rings_vector.at(i));
        succ_all.reserve(succ_all.size() + succ_ring.size());
        succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
    }

    double best_Q = -DBL_MAX;
    pair<pair<int,int>,pair<int,int>> best_move;
    for(auto& child : succ_all) {
        Board successor(state);
        successor.execute_move_and_return_server_string(child);
        double Q = q_value(state, successor);
        if(Q > best_Q) {
            best_Q = Q;
            best_move = child;
        }
    }

    return make_pair(best_Q, best_move);
}

vector<int> get_features(Board&s, Board& s_prime){
    int num_markers_p = s.num_markers;
    int num_rings_p = s.num_rings_on_board;
    int num_pairs_p = s.get_marker_rows(2, s.player_color).size();
    int num_triples_p = s.get_marker_rows(3, s.player_color).size();
    int num_quads_p = s.get_marker_rows(4, s.player_color).size();

    int num_markers_o = s.num_opp_markers;
    int num_rings_o = s.num_opp_rings_on_board;
    int num_pairs_o = s.get_marker_rows(2, s.other_color).size();
    int num_triples_o = s.get_marker_rows(3, s.other_color).size();
    int num_quads_o = s.get_marker_rows(4, s.other_color).size();

    // action features

    int num_markers_p_delta = s_prime.num_markers - num_markers_p;
    int num_rings_p_delta = s_prime.num_rings_on_board - num_rings_p;
    int num_pairs_p_delta = s_prime.get_marker_rows(2, s.player_color).size() - num_pairs_p;
    int num_triples_p_delta = s_prime.get_marker_rows(3, s.player_color).size() - num_triples_p;
    int num_quads_p_delta = s_prime.get_marker_rows(4, s.player_color).size() - num_quads_p;


    int num_markers_o_delta = s_prime.num_markers - num_markers_o;
    int num_rings_o_delta = s_prime.num_rings_on_board - num_rings_o;
    int num_pairs_o_delta = s_prime.get_marker_rows(2, s.player_color).size() - num_pairs_o;
    int num_triples_o_delta = s_prime.get_marker_rows(3, s.player_color).size() - num_triples_o;
    int num_quads_o_delta = s_prime.get_marker_rows(4, s.player_color).size() - num_quads_o;

    int num_rows_formed_p = s_prime.get_marker_rows(5, s.player_color).size();
    int num_rows_formed_o = s_prime.get_marker_rows(5, s.other_color).size();

    vector<int> features;

    features.emplace_back(num_markers_p);
    features.emplace_back(num_rings_p);
    features.emplace_back(num_pairs_p);
    features.emplace_back(num_triples_p);
    features.emplace_back(num_quads_p);
    features.emplace_back(num_markers_o);
    features.emplace_back(num_rings_o);
    features.emplace_back(num_pairs_o);
    features.emplace_back(num_triples_o);
    features.emplace_back(num_quads_o);
    features.emplace_back(num_markers_p_delta);
    features.emplace_back(num_rings_p_delta);
    features.emplace_back(num_pairs_p_delta);
    features.emplace_back(num_triples_p_delta);
    features.emplace_back(num_quads_p_delta);
    features.emplace_back(num_markers_o_delta);
    features.emplace_back(num_rings_o_delta);
    features.emplace_back(num_pairs_o_delta);
    features.emplace_back(num_triples_o_delta);
    features.emplace_back(num_quads_o_delta);
    features.emplace_back(num_rows_formed_p);
    features.emplace_back(num_rows_formed_o);
    return features;
}

void Agent::update_weights(double difference, Board& state, Board& successor){
    vector<int> features = get_features(state, successor);

    for(int i = 0; i < feature_weights.size(); i++) {
        feature_weights.at(i) = feature_weights.at(i) + learning_rate * difference * features.at(i);
    }
}

bool Agent::Qiteration(Board& state) {
    vector<pair<pair<int, int>, pair<int, int> > > succ_all;
    for (int i = 0; i < state.num_rings_on_board; i++) {
        vector<pair<pair<int, int>, pair<int, int> > > succ_ring = state.successors(state.rings_vector.at(i));
        succ_all.reserve(succ_all.size() + succ_ring.size());
        succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
    }

    double coin = ((double) rand() / (RAND_MAX));

    int idx = 0;
    double best_V = -DBL_MAX;

    //epsilon-greedy
    Board successor(state);
    if (coin < epsilon_exploration) {
        cerr << "Random move" << endl;
        //random action
        idx = rand() % succ_all.size();
        best_V = get_best_Qmove(successor).first;
    }
    else {
        // choose best successor
        cerr << "Greedy move" << endl;
        for(int i = 0; i < succ_all.size(); i++) {
            Board temp(state);
            temp.execute_move_and_return_server_string(succ_all.at(i));
            double V = get_best_Qmove(temp).first;
            if(V > best_V) {
                best_V = V;
                idx = i;
            }
        }
    }
    successor.execute_move_and_return_server_string(succ_all.at(idx));
    cerr << "best_V " << best_V << endl;
    double Q_val = q_value(state, successor);
    cerr << "Q(s,a) = " << Q_val << endl;
    double reward = reward_function(state, successor);
    cerr << "r = " << reward << endl;
    double difference = (reward + discount_factor * best_V) - Q_val;
    cerr << "diff = " << difference << endl;
    update_weights(difference, state, successor);

    return check_end(successor) > 0;
}

void Agent::save_weights(string filename) {
    ofstream myfile(filename);
    if (myfile.is_open())
    {
        for(auto& weight: feature_weights)
            myfile << weight << "\n";
        myfile.close();
    }
}

void Agent::load_weights(string filename) {
    string line;
    ifstream myfile(filename);
    if(myfile.is_open()) {
        int i = 0;
        while(getline(myfile, line)) {
            feature_weights.at(i) = stod(line);
            i++;
        }
    }
}

double Agent::q_value(Board& s, Board& s_prime){
    // state features
    vector<int> features = get_features(s, s_prime);
    return inner_product(begin(feature_weights), end(feature_weights), begin(features), 0);
}

void Agent::print_weights() {
    for(auto&weight : feature_weights) {
        cerr << weight << "; ";
    }
    cerr << endl;
}

double Agent::reward_function(Board& s, Board& s_prime){
    int res = check_end(s_prime);
    if (res > 0)
        return res;
    else
        return 0;
}

double Agent::minimax_ab(Board board, Node *node, int depth, double alpha, double beta, int maxDepth) {
    if (depth == maxDepth)
        return board.calculate_score();
    else {
        node->isLeaf = false;
        vector<pair<pair<int, int>, pair<int, int> > > succ_all;
        if (depth % 2 == 0) {//Self player is playing
            node->type = 'M';
            for (int i = 0; i < state.num_rings_on_board; i++) {
                vector<pair<pair<int, int>, pair<int, int> > > succ_ring = board.successors(board.rings_vector.at(i));
                succ_all.reserve(succ_all.size() + succ_ring.size());
                succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
            }
        }
        else {//Opponent is playing
            node->type = 'm';
            for (int i = 0; i < state.num_opp_rings_on_board; i++) {
                vector<pair<pair<int, int>, pair<int, int> > > succ_ring = board.successors(board.opp_rings_vector.at(i));
                succ_all.reserve(succ_all.size() + succ_ring.size());
                succ_all.insert(succ_all.end(), succ_ring.begin(), succ_ring.end());
            }
        }

        node->children = new Node*[succ_all.size()];

        if(node->type == 'M') {
            double v = -INFINITY;
            for (int i = 0; i < succ_all.size(); i++) {
                node->children[i] = new Node;
                node->children[i]->move = succ_all[i];
                node->children[i]->type = 'm';
                Board temp_board(board);
                temp_board.move_ring(succ_all[i].first, succ_all[i].second);
                double v_prime = minimax_ab(temp_board, node->children[i], depth + 1, v, beta, maxDepth);
                if(v_prime > v){
                    v = v_prime;
                    node->gotoidx = i;
                }
                alpha = max(alpha, v);
                if (alpha >= beta)
                    break;
            }
            return v;
        }
        else {
            double v = INFINITY;
            for (int i = 0; i < succ_all.size(); i++) {
                node->children[i] = new Node;
                node->children[i]->move = succ_all[i];
                node->children[i]->type = 'M';
                Board temp_board(board);
                temp_board.move_ring(succ_all[i].first, succ_all[i].second);
                double v_prime = minimax_ab(temp_board, node->children[i], depth + 1, alpha, v, maxDepth);
                if(v_prime < v) {
                    v = v_prime;
                    node->gotoidx = i;
                }
                beta = min(beta, v);
                if (alpha >= beta)
                    break;
            }
            return v;
        }
    }
}


//// Copy state to state_tree
//void Agent::copy_board() {
//    // = operator copies maps and copies vectors
//    state_tree.game_board = state.game_board;
//    state_tree.num_rings_on_board = state.num_rings_on_board;
//    state_tree.num_opp_rings_on_board = state.num_opp_rings_on_board;
//    state_tree.rings_vector = state.rings_vector;
//    state_tree.opp_rings_vector = state.opp_rings_vector;
//
//}

//// Simple scoring function based on Slovenian guy recommendation #2
//double Agent::score_function(vector<pair<pair<int, int>, pair<int, int> > > vec) {
//    vector<pair<pair<int, int>, pair<int, int> > >::iterator ptr;
//
//    double result = 0;
//    for (ptr = vec.begin(); ptr < vec.end(); ptr++) {
//        pair<pair<int, int>, pair<int, int> > tuple = *ptr;
//        pair<int, int> start = tuple.first;
//        pair<int, int> end = tuple.second;
//        if (start.first == end.first) // x coordinate same case
//            result += pow(3.0, (fabs(end.second - start.second) + 1)) - 1;
//        else if (start.second == end.second) // y coordinate same case
//            result += pow(3.0, (fabs(end.first - start.first) + 1)) - 1;
//        else // x - y = k
//            result += pow(3.0, (fabs(end.first - start.first) + 1)) - 1;
//        // Can think of combining else and else if
//    }
//    return 0.5 * result;
//}
//
//// Calculate score of player, subtract score of opponent
//double Agent::calculate_score(Board& board) {
//    vector<pair<pair<int, int>, pair<int, int> > > player_markers = board.get_marker_rows(1, board.player_color);
//    vector<pair<pair<int, int>, pair<int, int> > > opp_markers = board.get_marker_rows(1, board.other_color);
//    double score = score_function(player_markers) - score_function(opp_markers);
//    vector<pair<pair<int, int>, pair<int, int> > > five_or_more = state.get_marker_rows(5, state.player_color);
//    if (!five_or_more.empty())
//      score += 100000;
//    five_or_more = state.get_marker_rows(5, state.other_color);
//    if (!five_or_more.empty())
//      score += -100000;
//    return score;
//}

//todo: clarify stalemate  and  timeout policy
