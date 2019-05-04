#include "lw_stream.h"
#include <iostream>
#include <algorithm>
#include <sstream>
using namespace lwlib;

const StreamException StreamExceptionFactory::unexpectedTag(
	const std::string &tag1, const std::string &tag2) const
{
	std::stringstream ss;
	ss << "unexpected tag " << tag1 << " instead of " << tag2;
	return StreamException(ss.str());
}

const StreamException StreamExceptionFactory::cannotParse(
	const std::string &word) const
{
	std::stringstream ss;
	ss << "cannot parse '" << word << "'";
	return StreamException(ss.str());
}

const StreamException StreamExceptionFactory::cannotFind(
	const std::string &path) const
{
	std::stringstream ss;
	ss << "cannot find " << path;
	return StreamException(ss.str());
}

// fileName format:
// <file1>;<file2>;...;<filen>:<pakfile>
Stream::Stream(const char *fileName, StreamDirection::e dir_, 
	StreamFormat::e format_, StreamChecksum *checksum_) : fp(NULL),
	ownsFileHandle(false), dir(dir_), format(format_), totalBytes(0),
	checksum(checksum_), parseSpaceDelimString(false),
	fpak(NULL)
{
	std::string mode;
	std::string specfull = fileName;
	std::string specfile = fileName;
	std::string specpak;

	{
		const char *p = strchr(fileName, ':');
		if (p != NULL)
		{
			specfile = std::string(fileName, p - fileName);
			specpak = p + 1;
		}
	}

	if (dir == StreamDirection::out)
	{
		mode += "w";
	}
	else if (dir == StreamDirection::in)
	{
		mode += "r";
	}
	if (format == StreamFormat::binary)
	{
		mode += "b";
	}

	typedef std::vector<std::string> SpecFileList;
	SpecFileList specfilelist;
	if (specfile.find(';') != std::string::npos)
	{
		std::stringstream ss(specfile);
		std::string tok;
		while (std::getline(ss, tok, ';'))
			specfilelist.push_back(tok);
	}
	else
	{
		specfilelist.push_back(specfile);
	}

	for (SpecFileList::const_iterator it = specfilelist.begin(); it != specfilelist.end(); ++it)
	{
		fileName = it->c_str();

		fp = fopen(fileName, mode.c_str());
		if (fp != NULL)
		{
			ownsFileHandle = true;
			break;
		}
		else
		{
			if (!specpak.empty() && (fpak = openPak(specpak.c_str())))
			{
				if (!checkPak(fpak) && (fp = pakSeekTo(fileName, fpak)))
				{
					ownsFileHandle = true;
					break;
				}
				else
				{
					closePak(fpak);
					fpak=NULL;
				}
			}
		}
	}
}

Stream::Stream(FILE *fp_, StreamDirection::e dir_, 
	StreamFormat::e format_, StreamChecksum *checksum_) :
	fp(fp_), ownsFileHandle(false), dir(dir_), format(format_),
	totalBytes(0), checksum(checksum_), parseSpaceDelimString(false),
	fpak(NULL)
{
}

Stream::~Stream()
{
	if (ownsFileHandle && fpak != NULL)
	{
		closePak(fpak);
		fp = NULL;
	}

	if (ownsFileHandle && fp != NULL)
	{
		fclose(fp);
	}
}

std::string Stream::serializeTag(std::string name, 
	StreamTagBitMask tagBits)
{
	int ch;

	if (format == StreamFormat::binary)
	{
		return name;
	}

	if (dir == StreamDirection::out)
	{
		fprintf(
			fp, "%s<%s%s>%s", 
			(tagBits & StreamTagBits::addPadding) ? pad.c_str() : "", 
			(tagBits & StreamTagBits::isEnd) ? "/" : "",
			name.c_str(),
			(tagBits & StreamTagBits::addNewLine) ? "\n" : ""
			);
	}
	else
	{
		name.clear();
		if (!(tagss >> name))
		{
			tagss.clear();

			ch = fgetc(fp);
			while (ch != EOF && isspace(ch))
			{
				ch = fgetc(fp);
				if (ch == '#')
				{
					char buf[64];
					while (fgets(buf, sizeof(buf), fp) &&
						buf[strlen(buf) - 1] != '\n');
					ch = ' ';
				}
			}
			if (ch != EOF && ch == '<')
			{
				ch = fgetc(fp);
				while (ch != EOF && ch != '>')
				{
					if (ch != '/')
					{
						name.push_back(ch);
					}
					ch = fgetc(fp);
				}
			}
		}
		else
		{
			tagss.clear();
		}
	}

	return name;
}

std::string Stream::serializeStartTag(const std::string &name, StreamTagOpt::e optional)
{
	StreamTagBitMask bitMask = 0;
	bitMask |= StreamTagBits::addPadding;
	//if (optional)
	//	bitMask |= StreamTagBits::isOptional;
	return serializeTag(name, bitMask);
}

std::string Stream::serializeEndTag(const std::string &name)
{
	StreamTagBitMask bitMask = 0;
	bitMask |= StreamTagBits::addNewLine;
	bitMask |= StreamTagBits::isEnd;
	return serializeTag(name, bitMask);
}

bool Stream::isIn(void) const
{
	return dir == StreamDirection::in;
}

bool Stream::isOut(void) const
{
	return dir == StreamDirection::out;
}

namespace lwlib
{
	namespace StreamStartTag
	{
		Info::Info(const std::string &startTag)
		{
			std::stringstream ss(startTag);
			for (int i = 0; i < maxArgs; i++)
			{
				ss >> w[i];
			}
		}

		bool Info::findArg(const std::string &arg) const
		{
			return std::find(&w[0], &w[maxArgs], arg) != w + maxArgs;
		}

		bool Info::needsStore(void) const
		{
			return copy() || store();
		}

		bool Info::copy(void) const
		{
			return findArg("copy");
		}

		bool Info::noread(void) const
		{
			return findArg("noread");
		}

		bool Info::store(void) const
		{
			return findArg("store");
		}

		const std::string Info::getTag(void) const
		{
			return w[0];
		}

		const std::string Info::getStoreName(void) const
		{
			return w[2];
		}
	}

	void serialize(Stream &stream, int &x)
	{
		stream.serialize(x, "%d");
	}

	void serialize(Stream &stream, unsigned int &x)
	{
		stream.serialize(x, "%u");
	}

	void serialize(Stream &stream, short int &x)
	{
		stream.serialize(x, "%hd");
	}

	void serialize(Stream &stream, unsigned short &x)
	{
		stream.serialize(x, "%hu");
	}

	void serialize(Stream &stream, unsigned long &x)
	{
		stream.serialize(x, "%lu");
	}

	void serialize(Stream &stream, double &x)
	{
		stream.serialize(x, "%lf");
	}

	void serialize(Stream &stream, std::string &x)
	{
		int i, len, ch;

		if (stream.format == StreamFormat::binary)
		{
			if (stream.dir == StreamDirection::out)
			{
				len = (int)x.size();
				fwrite(&len, sizeof(len), 1, stream.fp);
				fwrite(x.c_str(), len, 1, stream.fp);
			}
			else
			{
				fread(&len, sizeof(len), 1, stream.fp);

				x.clear();
				for (i = 0; i < len; i++)
				{
					ch = fgetc(stream.fp);
					x.push_back((char)ch);
				}
			}
		}
		else if (stream.format == StreamFormat::text)
		{
			if (stream.dir == StreamDirection::out)
			{
				fprintf(stream.fp, "%s", x.c_str());
			}
			else
			{
				if (stream.parseSpaceDelimString)
				{
					x.clear();

					ch = fgetc(stream.fp);
					while (ch != EOF && isspace(ch))
						ch = fgetc(stream.fp);

					if (ch != EOF)
					{
						ungetc(ch, stream.fp);

						ch = fgetc(stream.fp);
						while (ch != EOF && !isspace(ch) && ch != '<')
						{
							x.push_back((char)ch);
							ch = fgetc(stream.fp);
						}
						if (isspace(ch) || ch == '<')
							ungetc(ch, stream.fp);
					}
				}
				else
				{
					x.clear();
					ch = fgetc(stream.fp);
					while (ch != EOF && (char)ch != '<')
					{
						if (ch == '\\')
						{
							ch = fgetc(stream.fp);
							if (ch == 'n')
								ch = '\n';
							else if (ch != '<')
							{
								ungetc(ch, stream.fp);
								ch = '\\';
							}
						}
						x.push_back((char)ch);
						ch = fgetc(stream.fp);
					}
					if (ch == '<')
					{
						ungetc(ch, stream.fp);
					}
				}

			}
		}
	}

	void serialize(Stream &stream, bool &x)
	{
		if (stream.format == StreamFormat::binary)
		{
			stream.serialize(x);
		}
		else if (stream.format == StreamFormat::text)
		{
			std::string s = x ? "true" : "false";
			serialize(stream, s);
			x = (strstr(s.c_str(), "true") != NULL ? true : false);
		}
	}

	void serialize(Stream &stream, unsigned char &x)
	{
		stream.serialize(x, "%hhd");
	}

	template <typename T>
	void serializePod(Stream &stream, std::vector<T> &x)
	{
		typedef typename std::vector<T>::size_type size_type;

		size_type count = x.size();
		stream & makenvp("count", count);

		if (stream.isIn())
		{
			x.clear();
			x.resize(count);
			fread(&x[0], sizeof(T), count, stream.fp);
		}
		else if (stream.isOut())
		{
			fwrite(&x[0], sizeof(T), count, stream.fp);
		}
	}

	void serialize(Stream &stream, std::vector<unsigned char> &x)
	{
		if (stream.format == StreamFormat::binary)
		{
			serializePod(stream, x);
		}
		else
		{
			serialize<>(stream, x);
		}
	}

	namespace StreamFormat
	{
		Permitted::Permitted() : len(0)
		{
		}

		Permitted::Permitted(enum e x) : len(1)
		{
			vals[0] = x;
		}

		Permitted::Permitted(enum e x, enum e y) : len(2)
		{
			vals[0] = x;
			vals[1] = y;
		}

		void Permitted::add(enum e x)
		{
			if (len >= 2)
			{
				throw "cannot insert format";
			}

			vals[len++] = x;
		}
		
		bool Permitted::enabled(enum e x) const
		{
			return 
				(
					(len == 2) ?
						(vals[0] == x || vals[1] == x) :
						(
							(len == 1) ?
								(vals[0] == x) :
								false
						)
				);
		}

		const Permitted allPermitted = Permitted(binary, text);
		const Permitted binaryPermitted = Permitted(binary);
		const Permitted textPermitted = Permitted(text);
	}
}
