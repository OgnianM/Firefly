/*
    Firefly Chess Engine
    Copyright (C) 2022  Ognyan Mirev

    This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef FIREFLY_ENGINE_INTERFACE_H
#define FIREFLY_ENGINE_INTERFACE_H


#include <external/SenjoUCIAdapter/senjo/ChessEngine.h>
#include <engine/mcts/search.h>
#include <testing/chess_gui.h>
#include <external/cxxopts/include/cxxopts.hpp>

struct engine_interface : public senjo::ChessEngine
{
    //BoardGUI gui;
    mcts::search search;
    std::list<senjo::EngineOption> engine_options;
    bool debug;

    engine_interface(cxxopts::ParseResult& options);
    virtual std::string getEngineName() const;

    //---------------------------------------------------------------------------
    //! \brief Get the engine version (e.g. "major.minor.build" e.g. "1.0.0")
    //! \return The engine version
    //---------------------------------------------------------------------------
    virtual std::string getEngineVersion() const;

    //---------------------------------------------------------------------------
    //! \brief Get the engine author name(s)
    //! \return The engine author name(s)
    //---------------------------------------------------------------------------
    virtual std::string getAuthorName() const;

    //---------------------------------------------------------------------------
    //! \brief Get email address(es) for use with this engine
    //! Return an empty string if you don't wish to report an email address.
    //! \return An email address(es) for use with this engine
    //---------------------------------------------------------------------------
    //virtual std::string getEmailAddress() const;

    //---------------------------------------------------------------------------
    //! \brief Get the name of the country this engine originates from
    //! Return an empty string if you don't wish to report a country
    //! \return The name of the country this engine originates from
    //---------------------------------------------------------------------------
    //virtual std::string getCountryName() const;

    //---------------------------------------------------------------------------
    //! \brief Get options supported by the engine, and their current values
    //! \return A list of the engine's options and their current values
    //---------------------------------------------------------------------------
    virtual std::list<senjo::EngineOption> getOptions() const;

    //---------------------------------------------------------------------------
    //! \brief Set a particular option to a given value
    //! Option value may be empty, particularly if the option type is Button
    //! \param[in] optionName The option name
    //! \param[in] optionValue The new option value
    //! \return false if the option name or value is invalid
    //---------------------------------------------------------------------------
    virtual bool setEngineOption(const std::string& optionName,
                                 const std::string& optionValue);

    //---------------------------------------------------------------------------
    //! \brief Initialize the engine
    //---------------------------------------------------------------------------
    virtual void initialize() ;

    //---------------------------------------------------------------------------
    //! \brief Is the engine initialized?
    //! \return true if the engine is initialized
    //---------------------------------------------------------------------------
    virtual bool isInitialized() const;

    //---------------------------------------------------------------------------
    //! \brief Set the board position according to a given FEN string
    //! The engine should use Output() to report errors in the FEN string.
    //! Only use position info from the given FEN string, don't process any moves
    //! or other data present in the FEN string.
    //! \param[in] fen The FEN string on input
    //! \param[out] remain If not NULL populated with tail portion of \p fen
    //!                    string that was not used to set the position.
    //! \return false if the FEN string does not contain a valid position
    //---------------------------------------------------------------------------
    virtual bool setPosition(const std::string& fen,
                             std::string* remain = nullptr);

    //---------------------------------------------------------------------------
    //! \brief Execute a single move on the current position
    //! Determine whether the given string is a valid move
    //! and if it is apply the move to the current position.
    //! Moves should be in coordinate notation (e.g. "e2e4", "g8f6", "e7f8q").
    //! \param[in] move A string containing move coordinate notation
    //! \return false if the given string isn't a valid move
    //---------------------------------------------------------------------------
    virtual bool makeMove(const std::string& move);

    //---------------------------------------------------------------------------
    //! \brief Get a FEN string representation of the current board position
    //! \return A FEN string representation of the current board postiion
    //---------------------------------------------------------------------------
    virtual std::string getFEN() const;

    //---------------------------------------------------------------------------
    //! \brief Output a text representation of the current board position
    //---------------------------------------------------------------------------
    virtual void printBoard() const;

    //---------------------------------------------------------------------------
    //! \brief Is it white to move in the current position?
    //! \return true if it is white to move in the current position
    //---------------------------------------------------------------------------
    virtual bool whiteToMove() const;

    //---------------------------------------------------------------------------
    //! \brief Clear any engine data that can persist between searches
    //! Examples of search data are the transposition table and killer moves.
    //---------------------------------------------------------------------------
    virtual void clearSearchData();

    //---------------------------------------------------------------------------
    //! \brief The last ponder move was played
    //! The Go() method may return a ponder move which is the expected response
    //! to the bestmove returned by Go().  If pondering is enabled the UCI
    //! adapter may tell the engine to ponder this move, e.g. start searching
    //! for a reply to the ponder move.  If, while the engine is pondering, the
    //! ponder move is played this method will be called.  In general the engine
    //! should make what it has learned from its pondering available for the next
    //! Go() call.
    //---------------------------------------------------------------------------
    virtual void ponderHit();

    //---------------------------------------------------------------------------
    //! \brief Is the engine registered?
    //! \return true if the engine is registered
    //---------------------------------------------------------------------------
    //virtual bool isRegistered() const;

    //---------------------------------------------------------------------------
    //! \brief Register the engine later
    //! The engine should still function, but may cripple itself in some fashion.
    //---------------------------------------------------------------------------
    //virtual void registerLater();

    //---------------------------------------------------------------------------
    //! \brief Register the engine now
    //! If this fails the engine should still function, but may cripple itself
    //! in some fashion.
    //! \param[in] name The name to register the engine to
    //! \param[in] code The code to use for registration
    //! \return true if successful
    //---------------------------------------------------------------------------
    //virtual bool doRegistration(const std::string& name,
     //                           const std::string& code);

    //---------------------------------------------------------------------------
    //! \brief Does this engine use copy protection?
    //! \return true if the engine uses copy protection
    //---------------------------------------------------------------------------
    //virtual bool isCopyProtected() const;

    //---------------------------------------------------------------------------
    //! \brief Determine whether this is a legitimate copy of the engine
    //! This method will be called if IsCopyProtected() returns true.  This is
    //! where your engine should try to determine whether it is a legitimate
    //! copy or not.
    //! \return true if the engine is a legitimate copy
    //---------------------------------------------------------------------------
    //virtual bool copyIsOK();

    //--------------------------------------------------------------------------
    //! \brief Set the engine's debug mode on or off
    //! \param[in] flag true to enable debug mode, false to disable debug mode
    //---------------------------------------------------------------------------
    virtual void setDebug(const bool flag);

    //---------------------------------------------------------------------------
    //! \brief Is debug mode enabled?
    //! \return true if debug mode is enabled
    //---------------------------------------------------------------------------
    virtual bool isDebugOn() const;

    //---------------------------------------------------------------------------
    //! \brief Is the engine currently executing the Go() method?
    //! It is not recommended to set this to true while Perft() is executing.
    //! \return true if the engine is searching
    //---------------------------------------------------------------------------
    virtual bool isSearching();

    //---------------------------------------------------------------------------
    //! \brief Tell the engine to stop searching
    //! Exit Perft()/Go() methods as quickly as possible.
    //---------------------------------------------------------------------------
    virtual void stopSearching();

    //--------------------------------------------------------------------------
    //! \brief Was stopSearching() called after the last go() or perft() call?
    //! \return true if stopSearching() called after the last go() or perft() call
    //--------------------------------------------------------------------------
    virtual bool stopRequested() const;

    //---------------------------------------------------------------------------
    //! \brief Block execution on the calling thread until the engine is
    //! finished searching.  Return immediateky if no search in progress.
    //---------------------------------------------------------------------------
    virtual void waitForSearchFinish();

    //---------------------------------------------------------------------------
    //! \brief Do performance test on the current position
    //! \param[in] depth How many half-moves (plies) to search
    //! \return The number of leaf nodes visited at \p depth
    //---------------------------------------------------------------------------
    virtual uint64_t perft(const int depth);

    //---------------------------------------------------------------------------
    //! \brief Execute search on current position to find best move
    //! \param[in] params UCI "go" command parameters
    //! \param[out] ponder If not null set to the move engine should ponder next
    //! \return Best move in coordinate notation (e.g. "e2e4", "g8f6", "e7f8q")
    //---------------------------------------------------------------------------
    virtual std::string go(const senjo::GoParams& params,
                           std::string* ponder = nullptr);

    //--------------------------------------------------------------------------
    //! \brief Get statistics about the last (or current) search
    //! \param[in] count The maximum number of lines to get stats for
    //! \return a SearchStats struct updated with the latest search stats
    //--------------------------------------------------------------------------
    virtual senjo::SearchStats getSearchStats() const;

    //--------------------------------------------------------------------------
    //! \brief Reset custom engine statistical counter totals
    //! \remark Override if you desire custom engine stats to be output when
    //!         the "test" command is run.
    //--------------------------------------------------------------------------
   // virtual void resetEngineStats();

    //--------------------------------------------------------------------------
    //! \brief Output engine stats collected since last resetEngineStats call
    //! \remark Override if you desire custom engine stats to be output when
    //!         the "test" command is run.
    //--------------------------------------------------------------------------
    //virtual void showEngineStats() const;
};



#endif //FIREFLY_ENGINE_INTERFACE_H
