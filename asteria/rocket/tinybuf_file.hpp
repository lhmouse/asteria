// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_FILE_HPP_
#define ROCKET_TINYBUF_FILE_HPP_

#include "tinybuf.hpp"
#include "linear_buffer.hpp"
#include "unique_posix_file.hpp"
#include "unique_posix_fd.hpp"
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::close()
#include <stdio.h>  // ::fdopen(), ::fclose()
#include <errno.h>  // errno

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocT = allocator<charT>>
class basic_tinybuf_file;

template<typename charT, typename traitsT, typename allocT>
class basic_tinybuf_file
  : public basic_tinybuf<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using tinybuf_type  = basic_tinybuf<charT, traitsT>;
    using file_buffer   = basic_linear_buffer<char_type, traits_type, allocator_type>;
    using handle_type   = typename posix_file_closer::handle_type;
    using closer_type   = typename posix_file_closer::closer_type;

    using seek_dir   = typename tinybuf_type::seek_dir;
    using open_mode  = typename tinybuf_type::open_mode;
    using int_type   = typename tinybuf_type::int_type;
    using off_type   = typename tinybuf_type::off_type;
    using size_type  = typename tinybuf_type::size_type;

  private:
    basic_linear_buffer<char_type, traits_type, allocator_type> m_gbuf;  // input buffer
    off_type m_goff = -1;  // file position of the beginning of the input buffer
                           // [`-1` if the buffer is inactive; `-2` if the file is non-seekable.]
    unique_posix_file m_file = { nullptr, nullptr };  // file handle

  public:
    basic_tinybuf_file()
    noexcept(is_nothrow_constructible<file_buffer>::value)
      : m_gbuf()
      { }

    explicit
    basic_tinybuf_file(const allocator_type& alloc)
    noexcept
      : m_gbuf(alloc)
      { }

    basic_tinybuf_file(unique_posix_file&& file, const allocator_type& alloc = allocator_type())
    noexcept
      : basic_tinybuf_file(alloc)
      { this->reset(::std::move(file));  }

    basic_tinybuf_file(handle_type fp, closer_type cl, const allocator_type& alloc = allocator_type())
    noexcept
      : basic_tinybuf_file(alloc)
      { this->reset(fp, cl);  }

    basic_tinybuf_file(const char* path, open_mode mode, const allocator_type& alloc = allocator_type())
      : basic_tinybuf_file(alloc)
      { this->open(path, mode);  }

    ~basic_tinybuf_file()
    override;

    basic_tinybuf_file(basic_tinybuf_file&&)
      = default;

    basic_tinybuf_file&
    operator=(basic_tinybuf_file&&)
      = default;

  private:
    basic_tinybuf_file&
    do_xsync_gbuf(const char_type*& gcur, const char_type*& gend)
      {
        // Note that this function may or may not clear the get area.
        if(this->m_goff < 0)
          // If the input buffer is inactive or the file is non-seekable, don't do anything.
          return *this;

        if(gcur != gend) {
          // If the get area has been consumed partially, rewind the file position to the beginning of it,
          // then discard characters that have been consumed so far.
          ROCKET_ASSERT(gend == this->m_gbuf.end());
          if(::fseeko(this->m_file, this->m_goff, SEEK_SET) != 0) {
            // If the seek operation fails, mark the file non-seekable and preserve the get area.
            this->m_goff = -2;
            return *this;
          }

          // Discard all characters preceding `*gcur`.
          auto nbump = static_cast<size_type>(gcur - this->m_gbuf.begin());
          if(traits_type::fgetn(this->m_file, this->m_gbuf.mut_begin(), nbump) != nbump)
            noadl::sprintf_and_throw<runtime_error>("tinybuf_file: read error (errno `%d`, fileno `%d`)",
                                                    errno, ::fileno(this->m_file));
        }

        // Deactivate the input buffer and clear the get area.
        this->m_goff = -1;
        gcur = nullptr;
        gend = nullptr;
        return *this;
      }

  protected:
    off_type
    do_fortell()
    const override
      {
        if(!this->m_file)
          // No file has been opened. There are no characters to read.
          return -1;

        if(::feof(this->m_file) || ::ferror(this->m_file))
          // No characters could be read.
          return -1;

        // TODO: Not implemented yet.
        return 0;
      }

    basic_tinybuf_file&
    do_flush(const char_type*& gcur, const char_type*& gend, char_type*& /*pcur*/, char_type*& /*pend*/)
    override
      {
        if(gcur) {
          // If the get area exists, a file must have been opened.
          ROCKET_ASSERT(this->m_file);
          // Synchronize the input buffer.
          this->do_xsync_gbuf(gcur, gend);
        }

        // Notice that we don't use put areas.
        if(this->m_file) {
          // Although ISO C says flushing an update stream whose most recent operation is an input operation
          // results in undefined behavior, such effects are actually specified by POSIX-2008. It should be
          // safe to just call `fflush()` without checking.
          ::fflush(this->m_file);
        }

        return *this;
      }

    off_type
    do_seek(off_type off, seek_dir dir)
    override
      {
        // Invalidate the get area before doing anything else.
        this->do_sync_areas();

        // Normalize the `whence` argument.
        int whence;
        if(dir == tinybuf_type::seek_set)
          whence = SEEK_SET;
        else if(dir == tinybuf_type::seek_end)
          whence = SEEK_END;
        else
          whence = SEEK_CUR;

        // Reposition the file.
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>("tinybuf_file: no file opened");

        if(::fseeko(this->m_file, off, whence) != 0)
          noadl::sprintf_and_throw<runtime_error>("tinybuf_file: seek error (errno `%d`, fileno `%d`)",
                                                  errno, ::fileno(this->m_file));

        // Return the new offset.
        return ::ftello(this->m_file);
      }

    int_type
    do_underflow(const char_type*& gcur, const char_type*& gend, bool peek)
    override
      {
        // If the get area exists, update the offset and clear it.
        this->do_sync_areas();

        // Load more characters into the input buffer.
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>("tinybuf_file: no file opened");

        // Get the file position of the beginning of the new get area.
        auto goff = this->m_goff;
        if(goff != -2) {
          // The stream looks seekable.
          goff = ::ftello(this->m_file);
          // Perform a zero seek in case of interleaving reads and writes.
          if((goff == -1) || (::fseeko(this->m_file, 0, SEEK_CUR) != 0))
            // Mark the file non-seekable.
            goff = -2;
        }

        // Reserve storage for the read operation.
        size_type navail;
        if(!gcur) {
          // The file is seekable so the buffer may be reloaded.
          // In this case we discard the entire buffer.
          this->m_gbuf.clear();
          navail = this->m_gbuf.reserve(0x4000);
        }
        else {
          // The file is non-seekable so the buffer cannot be cleared.
          // In this case we discard characters that have been read so far.
          this->m_gbuf.discard(static_cast<size_type>(gcur - this->m_gbuf.begin()));
          // Reallocate the buffer as needed.
          navail = this->m_gbuf.reserve(0x2000);
          gcur = this->m_gbuf.begin();
          gend = this->m_gbuf.end();
        }

        // Read some characters and append them to the buffer.
        navail = traits_type::fgetn(this->m_file, this->m_gbuf.mut_end(), navail);
        this->m_gbuf.accept(navail);
        this->m_goff = goff;
        // Check for read errors.
        if((navail == 0) && ::ferror(this->m_file))
          noadl::sprintf_and_throw<runtime_error>("tinybuf_file: read error (errno `%d`, fileno `%d`)",
                                                  errno, ::fileno(this->m_file));

        // Get the number of characters available in total.
        navail = this->m_gbuf.size();
        if(navail == 0)
          // If no more characters are available, return EOF.
          return traits_type::eof();

        // Get the range of remaining characters.
        auto gbase = this->m_gbuf.begin();
        // Set the new get area. Exclude the first character if `peek` is not set.
        gcur = gbase + !peek;
        gend = gbase + navail;
        return traits_type::to_int_type(gbase[0]);
      }

    basic_tinybuf_file&
    do_overflow(char_type*& /*pcur*/, char_type*& /*pend*/, const char_type* sadd, size_type nadd)
    override
      {
        // If the get area exists, update the offset and clear it.
        this->do_sync_areas();

        // Notice that we don't use put areas.
        // Write the string directly to the file.
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>("tinybuf_file: no file opened");

        // Optimize the one-character case a little.
        bool succ = ROCKET_EXPECT(nadd == 1)
                        ? (traits_type::is_eof(traits_type::fputc(this->m_file, sadd[0])) == false)
                        : (traits_type::fputn(this->m_file, sadd, nadd) == nadd);
        if(!succ)
          noadl::sprintf_and_throw<runtime_error>("tinybuf_file: write error (errno `%d`, fileno `%d`)",
                                                  errno, ::fileno(this->m_file));

        return *this;
      }

  public:
    handle_type
    get_handle()
    const noexcept
      { return this->m_file.get();  }

    const closer_type&
    get_closer()
    const noexcept
      { return this->m_file.get_closer();  }

    closer_type&
    get_closer()
    noexcept
      { return this->m_file.get_closer();  }

    basic_tinybuf_file&
    reset(unique_posix_file&& file)
    noexcept
      {
        this->do_purge_areas();
        // Discard the input buffer and reset the file handle, ignoring any errors.
        this->m_goff = -1;
        this->m_file = ::std::move(file);
        return *this;
      }

    basic_tinybuf_file&
    reset(handle_type fp, closer_type cl)
    noexcept
      {
        this->do_purge_areas();
        // Discard the input buffer and reset the file handle, ignoring any errors.
        this->m_goff = -1;
        this->m_file.reset(fp, cl);
        return *this;
      }

    basic_tinybuf_file&
    open(const char* path, open_mode mode)
      {
        int flags;
        char mstr[8];
        unsigned mlen = 0;

        // Translate read/write access flags.
        if(tinybuf_base::has_mode(mode, tinybuf_base::open_read_write)) {
          flags = O_RDWR;
          mstr[mlen++] = 'r';
          mstr[mlen++] = '+';
        }
        else if(tinybuf_base::has_mode(mode, tinybuf_base::open_read)) {
          flags = O_RDONLY;
          mstr[mlen++] = 'r';
        }
        else if(tinybuf_base::has_mode(mode, tinybuf_base::open_write)) {
          flags = O_WRONLY;
          mstr[mlen++] = 'w';
        }
        else
          noadl::sprintf_and_throw<invalid_argument>("tinybuf_file: no access specified (path `%s`, mode `%u`)",
                                                     path, mode);

        // Translate combination flags.
        if(tinybuf_base::has_mode(mode, tinybuf_base::open_append)) {
          flags |= O_APPEND;
          mstr[0] = 'a';  // might be "w" or "r+"
        }
        if(tinybuf_base::has_mode(mode, tinybuf_base::open_binary)) {
          mstr[mlen++] = 'b';
        }
        if(tinybuf_base::has_mode(mode, tinybuf_base::open_create)) {
          flags |= O_CREAT;
        }
        if(tinybuf_base::has_mode(mode, tinybuf_base::open_truncate)) {
          flags |= O_TRUNC;
        }
        if(tinybuf_base::has_mode(mode, tinybuf_base::open_exclusive)) {
          flags |= O_EXCL;
        }
        mstr[mlen] = 0;

        // Open the file.
        unique_posix_fd fd(::open(path, flags, 0666), ::close);
        if(!fd)
          noadl::sprintf_and_throw<runtime_error>("tinybuf_file: open error (errno `%d`, path `%s`, mode `%u`)",
                                                  errno, path, mode);

        // Convert it to a `FILE*`.
        unique_posix_file file(::fdopen(fd, mstr), ::fclose);
        if(!file)
          noadl::sprintf_and_throw<runtime_error>("tinybuf_file: open error (errno `%d`, path `%s`, mode `%u`)",
                                                  errno, path, mode);
        // If `fdopen()` succeeds it will have taken the ownership of `fd`.
        fd.release();

        // Discard the input buffer and close the file, ignoring any errors.
        return this->reset(::std::move(file));
      }

    basic_tinybuf_file&
    close()
    noexcept
      {
        // Discard the input buffer and close the file, ignoring any errors.
        return this->reset(nullptr, nullptr);
      }

    basic_tinybuf_file&
    swap(basic_tinybuf_file& other)
    noexcept(is_nothrow_swappable<file_buffer>::value)
      {
        this->tinybuf_type::swap(other);

        noadl::xswap(this->m_gbuf, other.m_gbuf);
        noadl::xswap(this->m_goff, other.m_goff);
        noadl::xswap(this->m_file, other.m_file);
        return *this;
      }
  };

template<typename charT, typename traitsT, typename allocT>
basic_tinybuf_file<charT, traitsT, allocT>::
~basic_tinybuf_file()
  = default;

template<typename charT, typename traitsT, typename allocT>
inline
void
swap(basic_tinybuf_file<charT, traitsT, allocT>& lhs, basic_tinybuf_file<charT, traitsT, allocT>& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

extern template
class basic_tinybuf_file<char>;

extern template
class basic_tinybuf_file<wchar_t>;

using tinybuf_file   = basic_tinybuf_file<char>;
using wtinybuf_file  = basic_tinybuf_file<wchar_t>;

}  // namespace rocket

#endif
