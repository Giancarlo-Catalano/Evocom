//
// Created by gian on 25/09/22.
//

#ifndef DISS_SIMPLEPROTOTYPE_BREEDER_HPP
#define DISS_SIMPLEPROTOTYPE_BREEDER_HPP
#include "../../Utilities/utilities.hpp"
#include "../../names.hpp"
#include "../Recipe/Recipe.hpp"
#include "../../Random/RandomChance.hpp"
#include "../../Random/RandomElement.hpp"
#include "../../Random/RandomIndex.hpp"
#include <sstream>
#include <unordered_set>



namespace GC {

    class Breeder {
    public: //types
        using TList = Recipe::TList;
        using Index = size_t;
        using CrossoverBounds = std::pair<Index, Index>;
        using CrossoverRecipe = std::pair<CrossoverBounds, CrossoverBounds>;



    private:
        //for mutation
        RandomChance randomChanceOfMutation;
        FairCoin addOrRemoveElement;
        RandomElement<TCode> randomTCode;
        RandomElement<CCode> randomCCode;

        //for crossover
        RandomInt<Index> randomIndexChooser;
        RandomChance chooseCCodeCrossover;

        size_t minTransformAmount;
        size_t maxTransformAmount;

    public:


        Breeder(const Chance mutationChance, const Chance chanceOfCCodeCrossover, const size_t minTransformAmount, const size_t maxTransformAmount) :
            randomChanceOfMutation(mutationChance),
            addOrRemoveElement(),
            randomTCode(availableTCodes),
            randomCCode(availableCCodes),
            chooseCCodeCrossover(chanceOfCCodeCrossover),
            minTransformAmount(minTransformAmount),
            maxTransformAmount(maxTransformAmount){};


        Recipe mutate(const Recipe& individual) {
            //LOG("Mutating an individual");
            Recipe child = individual;
            mutateElements(child);
            mutateCCode(child);
            mutateLength(child); //adds or removes a transform
            return child;
        }

        Recipe crossover(const Recipe& A, const Recipe& B) {
            CrossoverRecipe recipe = generateValidCrossoverRecipe(A, B);

            TList newTList = crossoverLists(A.readTList(), B.readTList(), recipe.first, recipe.second);
            CCode newCCode = chooseCCodeCrossover.choose() ? A.readCCode() : B.readCCode();
            return Recipe{newTList, newCCode};
        }


        std::string to_string() {
            std::stringstream ss;
            ss<<"Breeder {";
            ss<<"MutationChance:"<<std::setprecision(2)<<randomChanceOfMutation.getChance();
            ss<<"}";
            return ss.str();
        }

        Chance getMutationRate() const {
            return randomChanceOfMutation.getChance();
        }

        void setMutationRate(const Chance newMutationRate) {
            randomChanceOfMutation.setChance(newMutationRate);
        }


    private:


        void addRandomElement(TList& tList) {
            Index position = randomIndexChooser.chooseInRange(0, tList.size());
            tList.insert(tList.begin()+position, randomTCode.choose());
        }

        void removeRandomElement(TList& tList) {
            Index position = randomIndexChooser.chooseInRange(0, tList.size()-1);
            tList.erase(tList.begin()+position);
        }

        void mutateElements(Recipe& individual) {
            for (TCode& tCode : individual.getTList())
                randomChanceOfMutation.doWithChance([&]{randomTCode.assignRandomValue(tCode);});
        }

        void mutateCCode(Recipe& individual) {
            randomChanceOfMutation.doWithChance([&](){
                randomCCode.assignRandomValue(individual.getCCode());});
        }

        bool canAddMoreTransforms(const Recipe& individual) {
            return individual.getTListLength()  < maxTransformAmount;
        }

        bool canRemoveTransforms(const Recipe& individual) {
            return individual.getTListLength() > minTransformAmount;
        }

        void mutateLength(Recipe& individual) {
            auto addTransform = [&]() -> void { addRandomElement(individual.getTList());};
            auto removeTransform = [&]() -> void { removeRandomElement(individual.getTList());};

            if (randomChanceOfMutation.choose()) {
                if (canAddMoreTransforms(individual)) {
                    if (canRemoveTransforms(individual))
                        addOrRemoveElement.doWithChanceOrElse(addTransform, removeTransform);
                    else
                        addTransform();
                }
                else {
                    if (canRemoveTransforms(individual))
                        removeTransform();
                }
            }
        }





        /**
         * Note that an empty selection is possible, and in that case the bounds should be (a, a)
         * If we wanted to have all the elements in a list of size s, it would be (0, s)
         * If we wanted to include only the first element, it would be (0, 1)
         * If we wanted to include the last element, it would be (s-1, s)
         * In case this is confusing, I think of the bounds as the "edges between cells in the lists"
         *    | a | b | c | d | e |
         *    0   1   2   3   4   5
         *
         *    It makes handling empty lists much easier
         *    This also assumes that the bounds are in the right direction
         * @tparam L the kind of collection that will be crossed over
         * @param X
         * @param Y
         * @param boundsX (index of the first element to be included, index of the last element to be included)
         * @param boundsY same as the other bounds
         * @return the list X, but the range of items in [boundsX] has been replaced with the items in [rangeY] of Y
         */
        template <class L>
        static L crossoverLists(const L& X, const L& Y, const CrossoverBounds& boundsX, const CrossoverBounds& boundsY) {
            L result;
            auto addFromBounds = [&](const L& collection, const Index start, const Index afterEnd) {
                std::copy(collection.begin()+start, collection.begin()+afterEnd, std::back_inserter(result));
            };

            addFromBounds(X, 0, boundsX.first);
            addFromBounds(Y, boundsY.first, boundsY.second);
            addFromBounds(X, boundsX.second, X.size());
            return result;
        }

        CrossoverRecipe generateValidCrossoverRecipe(const Recipe& A, const Recipe& B) {
            auto chooseIndex = [&](const Recipe& X) {
                return randomIndexChooser.chooseInRange(0, X.readTList().size()); //note that the size is also a possible option
            };
            auto orderBounds = [&](const CrossoverBounds& b) -> CrossoverBounds {
                return {std::min(b.first, b.second), std::max(b.first, b.second)};
            };
            auto chooseRandomBounds = [&]() -> CrossoverRecipe {
                return {orderBounds({chooseIndex(A), chooseIndex(A)}),
                        orderBounds({chooseIndex(B), chooseIndex(B)})};
            };
            auto predictLengthOfChild = [&](const CrossoverBounds& boundsA, const CrossoverBounds& boundsB) -> size_t {
                return A.getTListLength() - (boundsA.second - boundsA.first) + (boundsB.second-boundsB.first);
            };
            auto areBoundsAcceptable = [&](const CrossoverRecipe bounds) -> bool {
                //LOG("requested to check {(", bounds.first.first, ", ", bounds.first.second, "), (", bounds.second.first, ", ", bounds.second.second, ")}");
                //LOG("A has length ", A.getTListLength(), "while B has length", B.getTListLength());
                //LOG("The predicted final length is ", predictLengthOfChild(bounds.first, bounds.second));
                return isInInterval_inclusive(predictLengthOfChild(bounds.first, bounds.second), minTransformAmount, maxTransformAmount);
            };

            return retryUntil<CrossoverRecipe>(chooseRandomBounds, areBoundsAcceptable);
        };

    public:


        template <class T, class Generator> //generator is a T generate();
        static std::vector<T> generateUnique(const size_t howMany, const Generator& generator) {
            std::unordered_set<T> result;
            while (result.size() < howMany)
                result.insert(generator());

            std::vector<T> resultAsVector(result.begin(), result.end());
            return resultAsVector;
        }

        template <class T, class Generator> //generator is a T generate();
        static std::vector<T> generateUnique(const size_t howMany, const std::vector<T>& startingPoint, const Generator& generator) {
            size_t amountOfAttempts = 0;

            std::unordered_set<T> result(startingPoint.begin(), startingPoint.end());
            while (result.size() < howMany) {
                result.insert(generator());
                amountOfAttempts++;
            }

            std::vector<T> resultAsVector(result.begin(), result.end());
            return resultAsVector;
        }


        class RandomIndividual {
        private:
            RandomInt<size_t> randomLength;
            RandomElement<TCode> randomTCode{availableTCodes};
            RandomElement<CCode> randomCCode{availableCCodes};
        public:
            RandomIndividual(const size_t minTransformAmount, const size_t maxTransformAmount) :
                randomLength(minTransformAmount, maxTransformAmount){
            }


            RandomIndividual(const Breeder& breeder):
                randomLength(breeder.minTransformAmount, breeder.maxTransformAmount){
            }

            Recipe makeIndividual() {
                Recipe::TList tList(randomLength.choose());
                for (TCode& tCode: tList)
                    randomTCode.assignRandomValue(tCode);
                CCode cCode = randomCCode.choose();
                return Recipe(tList, cCode);
            }
        };


    };

} // GC

#endif //DISS_SIMPLEPROTOTYPE_BREEDER_HPP
