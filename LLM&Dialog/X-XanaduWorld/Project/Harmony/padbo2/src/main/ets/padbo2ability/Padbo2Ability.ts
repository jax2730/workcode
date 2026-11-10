/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import hilog from '@ohos.hilog';
import nativerender from 'libpadbo2.so';
import fomd from 'libfmodstudioL.so';

export enum ContextType {
  APP_LIFECYCLE,
  PAGE_LIFECYCLE,
}

let sub_windowClass = null;
if (nativerender == undefined)
{
  console.error('nativerender is null');
}
if (fomd == undefined)
{
  console.error('fomd is null');
}

const nativeAppLifecycle = nativerender.getContext(ContextType.APP_LIFECYCLE);


export default class EntryAbility extends UIAbility {
  destroySubWindow() {
    sub_windowClass.destroyWindow((err) => {
      if (err.code) {
        console.error('Failed to destroy the window. Cause: ' + JSON.stringify(err));
        return;
      }
      console.info('Succeeded in destroying the window.');
    });
  }

  onCreate(want, launchParam) {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onCreate');
    let assetRoot = this.context.filesDir + "/";
    let cacheRoot = this.context.cacheDir + "/";
    nativeAppLifecycle.onCreate(assetRoot, cacheRoot, this.context.resourceManager);
    globalThis.abilityWant = want;
  }

  onDestroy() {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    nativeAppLifecycle.onDestroy();
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

    let windowClass = null;
    windowStage.getMainWindow((err, data) => {

      if (err.code) {
        console.error('Failed to obtain the main window. Cause: ' + JSON.stringify(err));
        return;
      }

      windowClass = data;
      console.info('Succeeded in obtaining the main window. Data: ' + JSON.stringify(data));

      let names = [];
      windowClass.setWindowSystemBarEnable(names, (err) => {
        if (err.code) {
          console.error('Failed to set the system bar to be visible. Cause:' + JSON.stringify(err));
          return;
        }
        console.info('Succeeded in setting the system bar to be visible.');
      });

      windowClass.setWindowLayoutFullScreen(true)
    })

    globalThis.abilityContext = this.context
    globalThis.context = this.context
    nativeAppLifecycle.onShow();
    windowStage.loadContent('pages/Index', (err, data) => {
      if (err.code) {
        hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
        return;
      }
      hilog.info(0x0000, 'testTag', 'Succeeded in loading the content. Data: %{public}s', JSON.stringify(data) ?? '');
    });

    windowStage.on('windowStageEvent', (data) => {
      if (data == window.WindowStageEventType.ACTIVE) {
        fomd.init(this);
      }
      else if (data == window.WindowStageEventType.INACTIVE) {
        fomd.close();
      }
    });
  }

  onWindowStageDestroy() {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
    nativeAppLifecycle.onHide();
    // �ͷ�UI������Դ
  }

  onForeground() {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onForeground');
    // ����ϵͳ��Ҫ����Դ����������������onBackground���ͷŵ���Դ
  }

  onBackground() {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onBackground');
    // �ͷ�UI���治�ɼ�ʱ���õ���Դ�������ڴ˻ص���ִ�н�Ϊ��ʱ�Ĳ���������״̬�����
  }
};
