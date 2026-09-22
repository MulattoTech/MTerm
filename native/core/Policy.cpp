// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "Policy.h"
#include <QSet>
#include <limits>
namespace mterm {
namespace {
const QSet<QString> reads{"filesystem.read","git.read","process.read","system.info","workspace.read","task.read"};
const QSet<QString> writes{"filesystem.write","git.write","task.write","canvas.write"};
const QSet<QString> approvals{"terminal.execute","agent.execute","git.worktree","process.kill"};
}
Policy::Policy(QString workspace):workspace_(std::move(workspace)){}
void Policy::setWorkspace(QString workspace){ workspace_=std::move(workspace); developer_=false; revoke(); }
void Policy::setDeveloper(bool enabled){ if(developer_!=enabled) revoke(); developer_=enabled; }
bool Policy::allows(const QString &capability,qint64 nowMs) const {
 if(workspace_.isEmpty() || nowMs<0) return false;
 if(reads.contains(capability)) return true;
 if(!developer_) return false;
 if(writes.contains(capability)) return true;
 return approvals.contains(capability) && grants_.value(capability,-1)>nowMs;
}
bool Policy::grant(const QString &capability,qint64 nowMs,qint64 durationMs){
 constexpr qint64 maxDuration=8*60*60*1000;
 if(!developer_ || workspace_.isEmpty() || !approvals.contains(capability) || nowMs<0 ||
    durationMs<=0 || durationMs>maxDuration || nowMs>std::numeric_limits<qint64>::max()-durationMs) return false;
 grants_[capability]=nowMs+durationMs; return true;
}
void Policy::revoke(){ grants_.clear(); }
}
