#ifndef LWLIB_STREAM_H
#define LWLIB_STREAM_H

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <set>
#include <iostream>
#include "pak.h"

#define LWLIB_ENUMCASTTOINT(x) *((int *)&(x))

namespace lwlib
{
	namespace StreamDirection
	{
		enum e
		{
			in,
			out,
		};
	}

	namespace StreamFormat
	{
		enum e
		{
			binary,
			text,
		};

		class Permitted
		{
			enum e vals[2];
			int len;

		public:
			Permitted();

			explicit Permitted(enum e x);

			Permitted(enum e x, enum e y);

			void add(enum e x);

			bool enabled(enum e x) const;
		};

		extern const Permitted allPermitted;
		extern const Permitted binaryPermitted;
		extern const Permitted textPermitted;
	}

	namespace StreamTagBits
	{
		enum e
		{
			addPadding = 0x01,
			addNewLine = 0x02,
			isEnd = 0x04,
			isOptional = 0x08,
		};
	}

	typedef int StreamTagBitMask;

	namespace StreamTagOpt
	{
		enum e
		{
			no,
			yes,
		};
	}

	namespace StreamTagResetValue
	{
		enum e
		{
			no,
			yes,
		};
	}

	template <typename T>
	class StreamNvp
	{
	public:
		std::string name;
		T &val;
		StreamFormat::Permitted perm;
		StreamTagOpt::e optional;
		StreamTagResetValue::e resetValue;

		StreamNvp(std::string name_, T &val_, StreamFormat::Permitted perm_,
			StreamTagOpt::e optional_, StreamTagResetValue::e resetValue_) :
			name(name_), val(val_), perm(perm_), optional(optional_),
			resetValue(resetValue_)
		{
		}
	};

	template <typename T>
	StreamNvp<T> makenvp(std::string name, T &val)
	{
		StreamTagOpt::e optional = StreamTagOpt::no;
		if (!name.empty() && name[0] == '?')
		{
			name = name.substr(1);
			optional = StreamTagOpt::yes;
		}

		StreamTagResetValue::e resetValue = StreamTagResetValue::yes;
		if (!name.empty() && name[0] == '*')
		{
			name = name.substr(1);
			resetValue = StreamTagResetValue::no;
		}

		return StreamNvp<T>(name, val, StreamFormat::allPermitted,
			optional, resetValue);
	}

	template <typename T>
	StreamNvp<T> makenvp_bin(std::string name, T &val)
	{
		return StreamNvp<T>(name, val, StreamFormat::binaryPermitted,
			StreamTagOpt::no, StreamTagResetValue::yes);
	}

	template <typename T>
	StreamNvp<T> makenvp_text(std::string name, T &val)
	{
		return StreamNvp<T>(name, val, StreamFormat::textPermitted,
			StreamTagOpt::no, StreamTagResetValue::yes);
	}

	class StreamChecksum
	{
	public:
		virtual ~StreamChecksum()
		{
		}

		virtual void update(void *ptr, size_t sz) = 0;
	};

	class StreamException
	{
	public:
		std::string s;

		explicit StreamException(const std::string &s_) : s(s_)
		{
		}
	};

	class StreamParseException : public StreamException
	{
	public:
		explicit StreamParseException(const std::string &s_) : StreamException(s_)
		{
		}
	};

	class StreamExceptionFactory
	{
	public:
		const StreamException unexpectedTag(const std::string &tag1, const std::string &tag2) const;

		const StreamException cannotParse(const std::string &word) const;

		const StreamException cannotFind(const std::string &path) const;
	};

	class StreamSchema
	{
	public:
		virtual bool allow(const std::string &name) const = 0;
	};

	class Stream
	{
	public:
		FILE *fp;
		bool ownsFileHandle;
		StreamDirection::e dir;
		StreamFormat::e format;
		std::string pad;
		int totalBytes;
		StreamChecksum *checksum;
		bool parseSpaceDelimString;
		pakfile *fpak;
		std::stringstream tagss;

		Stream(const char *fileName, StreamDirection::e dir, 
			StreamFormat::e format, StreamChecksum *checksum = NULL);

		Stream(FILE *fp, StreamDirection::e dir, 
			StreamFormat::e format, StreamChecksum *checksum = NULL);

		~Stream();

		std::string serializeTag(std::string name, 
			StreamTagBitMask tagBits);

		std::string serializeStartTag(const std::string &name, StreamTagOpt::e optional);

		std::string serializeEndTag(const std::string &name);

		template <typename T>
		void serialize(T &x, const char *fmt = NULL)
		{
			if (format == StreamFormat::binary)
			{
				if (dir == StreamDirection::out)
				{
					fwrite(&x, sizeof(T), 1, fp);
				}
				else
				{
					fread(&x, sizeof(T), 1, fp);
				}
				if (checksum != NULL)
				{
					checksum->update(&x, sizeof(T));
				}
				/*{
					int i;
					for (i = 0; i < sizeof(T); i++)
					{
						fprintf(stderr, "%02x ", 
							(int)*((unsigned char *)&x + i));
					}
					fprintf(stderr, "\n");
					totalBytes += sizeof(T);
				}*/
			}
			else if (format == StreamFormat::text && fmt != NULL)
			{
				if (dir == StreamDirection::out)
				{
					fprintf(fp, fmt, x);
				}
				else
				{
					if (fscanf(fp, fmt, &x) != 1)
					{
						throw StreamParseException("failed to parse item");
					}
				}
			}
		}

		bool isIn(void) const;

		bool isOut(void) const;
	};

	namespace StreamStartTag
	{
		enum e
		{
			maxArgs = 4,
		};

		class Info
		{
			bool findArg(const std::string &arg) const;

		public:
			std::string w[maxArgs];

			// startTag format
			// <tag> [(copy|store) <storeName>] [noread]
			explicit Info(const std::string &startTag);

			bool needsStore(void) const;

			bool copy(void) const;

			bool noread(void) const;

			bool store(void) const;

			const std::string getTag(void) const;

			const std::string getStoreName(void) const;
		};
	}

	template <typename T>
	void serialize(Stream &stream, StreamNvp<T> nvp)
	{
		std::string startTag, endTag;

		if (!nvp.perm.enabled(stream.format))
		{
			return;
		}

		if (stream.format == StreamFormat::text && stream.isIn())
		{
			startTag = stream.serializeStartTag(nvp.name, nvp.optional);

			StreamStartTag::Info startTagInfo(startTag);
			if (startTagInfo.getTag() != nvp.name && !nvp.optional)
			{
				throw StreamExceptionFactory().unexpectedTag(
					startTagInfo.getTag(), nvp.name);
			}
			if (startTagInfo.getTag() != nvp.name && nvp.optional)
			{
				//std::cerr << "mismatch tag " << startTagInfo.getTag() << " instead of " << nvp.name << std::endl;
				stream.tagss << startTagInfo.getTag();
				stream.tagss.clear();
				if (nvp.resetValue)
					nvp.val = T();
				return;
			}

			if (startTagInfo.needsStore())
			{
				static std::map<std::string, T> objStore;

				if (startTagInfo.copy())
				{
					nvp.val = objStore.at(startTagInfo.getStoreName());
				}

				if (!startTagInfo.noread())
				{
					stream & nvp.val;
				}

				if (startTagInfo.store())
				{
					objStore.insert(std::make_pair(startTagInfo.getStoreName(),
						nvp.val));
				}
			}
			else
			{
				if (!startTagInfo.noread())
				{
					stream & nvp.val;
				}
			}

			endTag = stream.serializeEndTag(nvp.name);
			if (endTag != startTagInfo.getTag())
			{
				throw StreamExceptionFactory().unexpectedTag(
					endTag, startTagInfo.getTag());
			}
		}
		else
		{
			startTag = stream.serializeStartTag(nvp.name, StreamTagOpt::no);
			if (startTag != nvp.name)
			{
				throw StreamExceptionFactory().unexpectedTag(
					startTag, nvp.name);
			}
			stream & nvp.val;
			endTag = stream.serializeEndTag(nvp.name);
			if (endTag != startTag)
			{
				throw StreamExceptionFactory().unexpectedTag(
					endTag, startTag);
			}
		}
	}

	void serialize(Stream &stream, int &x);

	void serialize(Stream &stream, unsigned int &x);

	void serialize(Stream &stream, short int &x);

	void serialize(Stream &stream, unsigned short &x);

	void serialize(Stream &stream, unsigned long &x);

	void serialize(Stream &stream, double &x);

	void serialize(Stream &stream, bool &x);

	void serialize(Stream &stream, unsigned char &x);

	void serialize(Stream &stream, std::string &x);

	template <typename T>
	void serialize(Stream &stream, std::vector<T> &x)
	{
		typedef typename std::vector<T>::size_type size_type;

		size_type count = x.size();
		stream & makenvp("count", count);

		if (stream.isIn())
		{
			x.clear();
			for (size_type i = 0; i < count; i++)
			{
				T y = T();
				stream & makenvp("item", y);
				x.push_back(y);
			}
		}
		else if (stream.isOut())
		{
			for (size_type i = 0; i < count; i++)
			{
				stream & makenvp("item", x[i]);
			}
		}
	}

	void serialize(Stream &stream, std::vector<unsigned char> &x);

	template <typename K, typename T>
	void serialize(Stream &stream, std::map<K, T> &x)
	{
		typedef typename std::map<K, T>::size_type size_type;
		typedef typename std::map<K, T>::iterator iterator_type;

		size_type count = x.size();
		stream & makenvp("count", count);

		if (stream.isIn())
		{
			x.clear();
			for (size_type i = 0; i < count; i++)
			{
				std::pair<K, T> y;
				stream & y;
				x.insert(x.begin(), y);
			}
		}
		else if (stream.isOut())
		{
			for (iterator_type it = x.begin(); it != x.end(); it++)
			{
				std::pair<K, T> y = *it;
				stream & y;
			}
		}
	}

	template <typename K, typename T>
	void serialize(Stream &stream, std::multimap<K, T> &x)
	{
		typedef typename std::multimap<K, T>::size_type size_type;
		typedef typename std::multimap<K, T>::iterator iterator_type;

		size_type count = x.size();
		stream & makenvp("count", count);

		if (stream.isIn())
		{
			x.clear();
			for (size_type i = 0; i < count; i++)
			{
				std::pair<K, T> y;
				stream & y;
				x.insert(x.begin(), y);
			}
		}
		else if (stream.isOut())
		{
			for (iterator_type it = x.begin(); it != x.end(); it++)
			{
				std::pair<K, T> y = *it;
				stream & y;
			}
		}
	}

	template <typename K, typename T>
	void serialize(Stream &stream, std::pair<K, T> &x)
	{
		stream & makenvp("first", x.first);
		stream & makenvp("second", x.second);
	}

	template <typename T>
	void serialize(Stream &stream, std::set<T> &x)
	{
		typedef typename std::set<T>::size_type size_type;
		typedef typename std::set<T>::iterator iterator_type;

		size_type count = x.size();
		stream & makenvp("count", count);

		if (stream.isIn())
		{
			x.clear();
			for (size_type i = 0; i < count; i++)
			{
				T y;
				stream & makenvp("item", y);
				x.insert(x.begin(), y);
			}
		}
		else if (stream.isOut())
		{
			for (iterator_type it = x.begin(); it != x.end(); ++it)
			{
				T y = *it;
				stream & makenvp("item", y);
			}
		}
	}


	template <typename Iter>
	void serialize(Stream &stream, Iter start, Iter end)
	{
		for (Iter it = start; it != end; ++it)
		{
			stream & makenvp("item", *it);
		}
	}

	template <typename T>
	void serializeEnum(Stream &stream, T &x)
	{
		int y = (int)x;
		serialize(stream, y);
		x = (T)y;
	}

	template <typename T>
	void serializeEnum(Stream &stream, StreamNvp<T> nvp)
	{
		std::string startTag, endTag;

		if (!nvp.perm.enabled(stream.format))
		{
			return;
		}

		startTag = stream.serializeStartTag(nvp.name, nvp.optional);
		if (startTag != nvp.name)
		{
			throw StreamExceptionFactory().unexpectedTag(
				startTag, nvp.name);
		}
		serializeEnum(stream, nvp.val);
		endTag = stream.serializeEndTag(nvp.name);
		if (endTag != startTag)
		{
			throw StreamExceptionFactory().unexpectedTag(
				endTag, startTag);
		}
	}

	template <typename T>
	void serializeEnumArr(Stream &stream, T *x, int n)
	{
		int i;

		for (i = 0; i < n; i++)
		{
			serializeEnum(stream, makenvp("item", x[i]));
		}
	}

	template <typename T>
	Stream &operator&(Stream &stream, T &x)
	{
		serialize(stream, x);
		return stream;
	}

	template <typename T>
	Stream &operator&(Stream &stream, StreamNvp<T> x)
	{
		serialize(stream, x);
		return stream;
	}
}

#endif
