// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "FileService.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QStringDecoder>
#include <stdexcept>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#endif
namespace mterm {
namespace {
[[noreturn]] void fail(const QString &message){throw std::runtime_error(message.toStdString());}
void rejectAliases(const QString &path){
 const QFileInfo info(path);
 if(info.isSymLink()) fail("Symbolic links are not permitted in native file operations");
#ifdef Q_OS_WIN
 const auto wide=path.toStdWString();
 const DWORD attributes=GetFileAttributesW(wide.c_str());
 if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
  fail("Reparse points are not permitted in native file operations");
 if(info.isFile()){
  HANDLE handle=CreateFileW(wide.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
    nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
  if(handle==INVALID_HANDLE_VALUE) fail("Cannot verify file identity");
  BY_HANDLE_FILE_INFORMATION identity{};
  const bool ok=GetFileInformationByHandle(handle,&identity)!=0;
  CloseHandle(handle);
  if(!ok || identity.nNumberOfLinks>1) fail("Hard-linked or unverifiable file identity");
 }
#else
 struct stat identity{};
 if(::lstat(QFile::encodeName(path).constData(),&identity)==0 && S_ISREG(identity.st_mode) && identity.st_nlink>1)
  fail("Hard-linked files are not permitted");
#endif
}
QString digest(const QByteArray &bytes){return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());}
}
FileService::FileService(QString root){
 rejectAliases(root);
 const QFileInfo info(root);
 if(!info.isDir()) fail("Workspace must be an existing directory");
 root_=info.canonicalFilePath();
 if(root_.isEmpty()) fail("Cannot canonicalize workspace");
}
QString FileService::resolve(const QString &relative,bool writing) const {
 if(relative.isEmpty() || relative.size()>4096 || relative.contains(QChar::Null)) fail("Invalid relative path");
 QString normalized=relative; normalized.replace('\\','/');
 if(normalized==".") return root_;
 if(QDir::isAbsolutePath(normalized) || normalized.startsWith('/') || normalized.contains(':')) fail("Absolute/drive/stream paths are forbidden");
 const auto parts=normalized.split('/');
 static const QRegularExpression device("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$",QRegularExpression::CaseInsensitiveOption);
 QString candidate=root_;
 for(qsizetype i=0;i<parts.size();++i){
  const auto &part=parts[i]; const auto lower=part.toLower();
  if(part.isEmpty() || part=="." || part==".." || part.endsWith('.') || part.endsWith(' ') ||
     device.match(part.section('.',0,0)).hasMatch()) fail("Ambiguous or reserved path component");
  if(lower==".git" || lower==".ssh" || lower==".gnupg" || lower==".env" || lower.startsWith(".env."))
   fail("Protected secret or repository metadata path");
  candidate=QDir(candidate).filePath(part); rejectAliases(candidate);
  const QFileInfo info(candidate);
  if(!info.exists()){
   if(writing && i==parts.size()-1) return candidate;
   fail("Path does not exist");
  }
  const auto canonical=info.canonicalFilePath();
#ifdef Q_OS_WIN
  constexpr auto sensitivity=Qt::CaseInsensitive;
#else
  constexpr auto sensitivity=Qt::CaseSensitive;
#endif
  if(canonical.compare(root_,sensitivity)!=0 && !canonical.startsWith(root_+'/',sensitivity)) fail("Path escapes the workspace");
  if(i<parts.size()-1 && !info.isDir()) fail("Parent is not a directory");
 }
 return candidate;
}
FileSnapshot FileService::read(const QString &relative) const {
 const auto path=resolve(relative); const QFileInfo info(path);
 if(!info.isFile() || info.size()>MaxBytes) fail("File exceeds the native text-size limit");
 QFile file(path); if(!file.open(QIODevice::ReadOnly)) fail("Cannot open workspace file");
 const auto bytes=file.read(MaxBytes+1);
 if(bytes.size()>MaxBytes || file.error()!=QFileDevice::NoError) fail("File changed size or failed while reading");
 if(bytes.contains('\0')) fail("Binary file is not a text document");
 QStringDecoder decoder(QStringDecoder::Utf8); const QString text=decoder(bytes);
 if(decoder.hasError()) fail("File is not valid UTF-8");
 return {relative,text,digest(bytes)};
}
FileSnapshot FileService::write(const QString &relative,const QString &text,const QString &expectedVersion) const {
 const auto bytes=text.toUtf8();
 if(bytes.size()>MaxBytes || bytes.contains('\0') || expectedVersion.isEmpty()) fail("Invalid file content/version");
 const auto path=resolve(relative,true);
 const auto verify=[&]{
  if(expectedVersion=="missing") {if(QFileInfo::exists(path)) fail("STALE_WRITE: target now exists");}
  else if(read(relative).version!=expectedVersion) fail("STALE_WRITE: file changed since opening");
 };
 verify();
 QSaveFile file(path); file.setDirectWriteFallback(false);
 if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size()) fail("Atomic file write failed");
 // Recheck immediately before replacement; external writers still require a future handle-anchored protocol.
 resolve(relative,true); verify();
 if(!file.commit()) fail("Atomic file commit failed");
 return {relative,text,digest(bytes)};
}
}
