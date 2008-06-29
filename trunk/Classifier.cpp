#include "Classifier.h"
#include "Utils/StreamTokenizer.h"
#include "IO/Serialization.h"

Classifier::Classifier(const char* shypFile)
{
	using namespace MultiBoost;
	using namespace nor_utils;

	// open file
	ifstream inFile(shypFile);
	if (!inFile.is_open())
	{
		cerr << "ERROR: Cannot open strong hypothesis file <" << shypFile << ">!" << endl;
		exit(EXIT_FAILURE);
	}

	// Declares the stream tokenizer
	nor_utils::StreamTokenizer st(inFile, "<>\n\r\t");

	// Move until it finds the multiboost tag
	if ( !UnSerialization::seekSimpleTag(st, "multiboost") )
	{
		// no multiboost tag found: this is not the correct file!
		cerr << "ERROR: Not a valid MultiBoost Strong Hypothesis file!!" << endl;
		exit(EXIT_FAILURE);
	}

	string rawTag;
	string tag, tagParam, tagValue;

	for ( ; ; )
	{
		// move until the next weak hypothesis
		if ( UnSerialization::seekParamTag(st, "weakhyp") )
		{
			WeakHyp wh;
			wh.alpha = UnSerialization::seekAndParseEnclosedValue<double>(st, "alpha");
			wh.column = UnSerialization::seekAndParseEnclosedValue<int>(st, "column");
			// get parities
			string tagParam, paramVal;
			string className;
			string tag, param;
			const string closeTag = "/vArray";
			UnSerialization::seekAndParseParamTag(st, "vArray", tagParam, paramVal);
			short tmpVal;
			while ( !nor_utils::cmp_nocase(tag, closeTag) && st.has_token() )
			{
				// seek the parameter "class"
				UnSerialization::parseParamTag(st.next_token(), tag, param, paramVal );
				if ( !nor_utils::cmp_nocase(tag, "class") )
					continue;

				// convert the string into a value
				istringstream ss( st.next_token() );
				ss >> tmpVal;

				wh.parities[paramVal] = tmpVal;
			}
			////
			wh.threshold = UnSerialization::seekAndParseEnclosedValue<double>(st, "threshold");
			_weakHyps.push_back(wh);
		}
		else
			break;
	}
}

Classifier::~Classifier(void)
{
}

string Classifier::Classify(const double *data)
{
	pair<string, double> winner;
	pair<string, double> minimum;

	// get votes
	votes.clear();
	for(int i=0; i<(int)_weakHyps.size(); ++i)
	{
		short sign = data[_weakHyps[i].column] > _weakHyps[i].threshold ? 1 : -1;

		for(map<string, short>::const_iterator itr = _weakHyps[i].parities.begin(); 
			itr != _weakHyps[i].parities.end(); ++itr) 
		{
			votes[itr->first] += (double)sign * (double)itr->second * (double)_weakHyps[i].alpha;
		}
	}

	// normalize and get winner
	winner.second = -65000;
	minimum.second = 65000;
	double norm = 0;
	for(map<string, double>::const_iterator itr = votes.begin(); 
			itr != votes.end(); ++itr) 
	{
		if(itr->second < minimum.second)
		{
			minimum.first = itr->first;
			minimum.second = itr->second;
		}
		if(itr->second > winner.second)
		{
			winner.first = itr->first;
			winner.second = itr->second;
		}
	}
	for(map<string, double>::const_iterator itr = votes.begin(); 
		itr != votes.end(); ++itr) 
	{
		votes[itr->first] -= minimum.second;
		norm += votes[itr->first]*votes[itr->first];
	}
	norm = sqrt(norm);
	for(map<string, double>::const_iterator itr = votes.begin(); 
			itr != votes.end(); ++itr) 
	{
		votes[itr->first] /= norm;
	}

	return winner.first;
}